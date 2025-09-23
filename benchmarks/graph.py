import argparse
import re
import sys
import pandas as pd
import matplotlib.pyplot as plt
from matplotlib.ticker import FuncFormatter, ScalarFormatter

def extract_size(name):
    """Extract trailing integer input size from a name like 'BM_hive/200000'."""
    m = re.search(r'/(\d+)(?:$|/|[^0-9])', str(name))
    if m:
        return int(m.group(1))
    m2 = re.search(r'(\d{3,})', str(name))
    return int(m2.group(1)) if m2 else None

def base_name(name):
    """Return benchmark base name without trailing '/<size>' and suffixes."""
    return re.sub(r'/(?:\d+).*$', '', str(name))

def humanize_unit(unit):
    if pd.isna(unit):
        return ''
    return str(unit)

def main():
    p = argparse.ArgumentParser(description="Plot Google Benchmark CSV (interval inputs).")
    p.add_argument('csv', help='Google Benchmark CSV file')
    p.add_argument('--metric', choices=['real_time', 'cpu_time', 'items_per_second'],
                   default='real_time', help='Metric to plot (default: real_time)')
    p.add_argument('--per_item', action='store_true', help='Normalize metric by input size (metric / input_size)')
    p.add_argument('--to-seconds', action='store_true',
                   help='Convert time metrics from ns/us/ms to seconds (only applies to real_time/cpu_time)')
    p.add_argument('--logx', action='store_true', default=False, help='Use log10 x-axis (default: linear)')
    p.add_argument('--save', help='Save plot to PNG file (if omitted, shows interactively)')
    p.add_argument('--figsize', type=str, default='10,6', help='Figure size WIDTH,HEIGHT (default 10,6)')
    args = p.parse_args()

    try:
        df = pd.read_csv(args.csv, comment='#', skip_blank_lines=True)
    except Exception as e:
        print(f"Failed to read CSV: {e}", file=sys.stderr)
        sys.exit(1)

    if 'name' not in df.columns:
        print("CSV doesn't contain a 'name' column.", file=sys.stderr)
        sys.exit(1)

    # Extract input sizes and base names
    df['input_size'] = df['name'].apply(extract_size)
    if df['input_size'].isnull().any():
        print("Warning: some input sizes could not be parsed from 'name'. These rows will be ignored.", file=sys.stderr)
        df = df.dropna(subset=['input_size'])

    df['input_size'] = df['input_size'].astype(int)
    df['bench_base'] = df['name'].apply(base_name)

    metric = args.metric
    if metric not in df.columns:
        print(f"CSV doesn't contain '{metric}' column.", file=sys.stderr)
        # If items_per_second missing, fall back to real_time if available
        if metric == 'items_per_second' and 'real_time' in df.columns:
            print("Falling back to 'real_time'.", file=sys.stderr)
            metric = 'real_time'
        else:
            sys.exit(1)

    # Detect time unit (if present) — take the most common non-null value
    time_unit = None
    if 'time_unit' in df.columns:
        nonnull_units = df['time_unit'].dropna().unique()
        time_unit = nonnull_units[0] if len(nonnull_units) > 0 else None

    # Convert to numeric
    df[metric] = pd.to_numeric(df[metric], errors='coerce')
    df = df.dropna(subset=[metric, 'input_size'])

    # Aggregate multiple repetitions (mean)
    agg = df.groupby(['bench_base', 'input_size'], as_index=False)[metric].mean()

    # Optionally normalize per item
    if args.per_item:
        agg[metric] = agg[metric] / agg['input_size']

    # If requested, convert time to seconds (only if metric is a time)
    if args.to_seconds and metric in ('real_time', 'cpu_time'):
        # detect time_unit and convert accordingly. Default: assume ns if unknown.
        unit = (time_unit or 'ns').lower()
        if unit in ('ns', 'nanoseconds', 'nanosecond'):
            factor = 1e9
        elif unit in ('us', 'microseconds', 'microsecond'):
            factor = 1e6
        elif unit in ('ms', 'milliseconds', 'millisecond'):
            factor = 1e3
        elif unit in ('s', 'sec', 'seconds', 'second'):
            factor = 1.0
        else:
            # unknown: assume ns but warn
            print(f"Warning: unknown time_unit '{unit}', assuming nanoseconds.", file=sys.stderr)
            factor = 1e9
        agg[metric] = agg[metric] / factor
        display_unit = 's'
    else:
        display_unit = humanize_unit(time_unit) if time_unit else ''

    # Pivot so that each benchmark is a column
    pivot = agg.pivot(index='input_size', columns='bench_base', values=metric)
    pivot = pivot.sort_index()

    if pivot.empty:
        print("No data available after parsing; exiting.", file=sys.stderr)
        sys.exit(1)

    # Prepare plot
    fig_w, fig_h = (float(x) for x in args.figsize.split(','))
    plt.figure(figsize=(fig_w, fig_h))

    for col in pivot.columns:
        plt.plot(pivot.index.values, pivot[col].values, label=col, marker='o', linewidth=1.5)

    plt.xlabel('Input size')
    ylabel = metric
    if args.per_item:
        ylabel += ' (per item)'
    if args.to_seconds and metric in ('real_time', 'cpu_time'):
        ylabel += f' (seconds)'
    elif display_unit:
        ylabel += f' ({display_unit})'

    plt.ylabel(ylabel)
    plt.title('Google Benchmark comparison')
    plt.legend()
    plt.grid(alpha=0.3)

    # Format x ticks with commas
    plt.gca().xaxis.set_major_formatter(FuncFormatter(lambda x, pos: f"{int(x):,}" if x == int(x) else f"{x:,.0f}"))

    # For y axis: use scientific notation if values are large
    y_max = pivot.max().max()
    if (y_max is not None) and (abs(y_max) >= 1e6):
        plt.gca().yaxis.set_major_formatter(ScalarFormatter(useMathText=True))
        plt.ticklabel_format(axis='y', style='sci', scilimits=(0,0))
    else:
        plt.gca().yaxis.set_major_formatter(FuncFormatter(lambda y, pos: f"{y:,.0f}" if abs(y) >= 1 else f"{y:.3g}"))

    if args.logx:
        plt.xscale('log', base=10)

    plt.tight_layout()

    if args.save:
        plt.savefig(args.save, dpi=300)
        print(f"Saved plot to {args.save}")
    else:
        plt.show()


if __name__ == '__main__':
    main()