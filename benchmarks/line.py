import sys
import os
import json
import matplotlib.pyplot as plt

def format_batch_size(batch):
    if batch < 1024:
        return f"{batch}"
    elif batch < 1024 * 1024:
        return f"{batch // 1024}K"
    else:
        return f"{batch // (1024 * 1024)}M"

def main():
    # ---- 1) Read & validate CLI argument ----
    if len(sys.argv) != 2:
        print(f"Usage: {sys.argv[0]} <data.json>")
        sys.exit(1)

    input_path = sys.argv[1]
    if not os.path.isfile(input_path):
        print(f"Error: File not found: {input_path}")
        sys.exit(1)

    # Derive output PNG name
    base_name   = os.path.splitext(os.path.basename(input_path))[0]
    output_png  = base_name + '.png'

    # ---- 2) Load JSON data ----
    try:
        with open(input_path, 'r') as f:
            data = json.load(f)
    except json.JSONDecodeError as e:
        print(f"Error: Failed to parse JSON: {e}")
        sys.exit(1)

    # ---- 3) Organize data by benchmark ----
    benchmarks = {}
    for result in data.get('results', []):
        name    = result.get('name')
        batch   = result.get('batch')
        elapsed = result.get('median(elapsed)')

        if name is None or batch is None or elapsed is None:
            # skip any malformed entries
            continue

        # convert to µs immediately
        time_us = elapsed * 1e6

        benchmarks.setdefault(name, {'batches': [], 'times': []})
        benchmarks[name]['batches'].append(batch)
        benchmarks[name]['times'].append(time_us)

    if not benchmarks:
        print("Error: No valid benchmark data found in JSON.")
        sys.exit(1)

    # ---- 4) Plot ----
    plt.figure(figsize=(12, 7))
    for name, vals in benchmarks.items():
        plt.plot(
            vals['batches'],
            vals['times'],
            marker='o',
            linestyle='-',
            linewidth=2,
            label=name
        )

    plt.xscale('log', base=2)
    plt.yscale('log')
    plt.xlabel('Batch Size', fontsize=12)
    plt.ylabel('Median Elapsed Time (µs)', fontsize=12)
    plt.title('Benchmark Results: Execution Time vs Batch Size', fontsize=14)

    # X-axis ticks from the collected batch sizes
    all_batches = sorted({b for v in benchmarks.values() for b in v['batches']})
    plt.xticks(all_batches, [format_batch_size(b) for b in all_batches], rotation=45)

    plt.legend(fontsize=10)
    plt.grid(True, which='both', linestyle='--', alpha=0.7)
    plt.tight_layout()

    # ---- 5) Save & display ----
    plt.savefig(output_png, dpi=300, bbox_inches='tight')
    print(f"Saved plot to {output_png}")
    plt.show()

if __name__ == '__main__':
    main()
