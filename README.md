# hive
A container that provides pointer/iterator stability and fast iteration. Insertion order is not preserved.

It's my own C implementation of Matt Bentley's [`plf::colony`](https://github.com/mattreecebentley/plf_colony) data structure.

However, There are some differences.

To list some:
- I use fixed bucket sizes, while `plf::colony` doubles the bucket size each time a new bucket is created.
- My `hive` doesn't have a reverse iterator.
- In the "skip field" I don't store the offset index. Instead I store the absolute index the next element is in.

## Benchmark

From my (very limited) benchmarking, it seems my `hive` has slightly faster iteration and insertion, and slower deletion.
I won't post any benchmark results because I'm afraid they might not be too accurate. You can find the benchmark I used in `benchmarks/bench.cpp`

## Example

```C
#include <stdio.h>

#define HIVE_TYPE int
#define HIVE_NAME my_hive
#define HIVE_IMPL
#include "hive.h"

int main()
{
    my_hive ints;
    my_hive_init(&ints);
    
    my_hive_put(&ints, 10);
    my_hive_put(&ints, 20);
    my_hive_iter thirty = my_hive_put(&ints, 30);
    my_hive_put(&ints, 40);
    my_hive_put(&ints, 50);
    
    for(my_hive_iter it = my_hive_begin(&ints) ; !my_hive_iter_eq(it, my_hive_end(&ints)) ; it = my_hive_iter_next(it))
    {
        printf("%d\n", *it.ptr);
    }
    
    // convenience macro for iteration
    HIVE_FOR_EACH(it, my_hive_begin(&ints), my_hive_end(&ints))
    {
        printf("%d\n", *it.ptr);
    }
    
    my_hive_iter_del(&ints, thirty);
    
    my_hive_deinit(&ints);
}
```

## API
```C
void             <HIVE_NAME>_init(<HIVE_NAME> *_hv);
<HIVE_NAME>      <HIVE_NAME>_clone(const <HIVE_NAME> *const _hv);
<HIVE_NAME>_iter <HIVE_NAME>_put(<HIVE_NAME> *hv, <HIVE_TYPE> _new_elm);
<HIVE_NAME>_iter <HIVE_NAME>_put_uninit(<HIVE_NAME> *hv);
void             <HIVE_NAME>_put_all(<HIVE_NAME> *_hv, const <HIVE_TYPE> *_elms, size_t _nelms);
<HIVE_NAME>_iter <HIVE_NAME>_del(<HIVE_NAME> *_hv, <HIVE_TYPE> *_elm);
void             <HIVE_NAME>_deinit(<HIVE_NAME> *_hv);

<HIVE_NAME>_iter <HIVE_NAME>_begin(const <HIVE_NAME> *_hv);
<HIVE_NAME>_iter <HIVE_NAME>_end(const <HIVE_NAME> *_hv);
<HIVE_NAME>_iter <HIVE_NAME>_iter_next(<HIVE_NAME>_iter _it);
<HIVE_NAME>_iter <HIVE_NAME>_iter_del(<HIVE_NAME> *_hv, <HIVE_NAME>_iter _it);
bool             <HIVE_NAME>_iter_eq(<HIVE_NAME>_iter _a, <HIVE_NAME>_iter _b);
<HIVE_NAME>_iter <HIVE_NAME>_ptr_to_iter(<HIVE_NAME> *_hive, <HIVE_TYPE> *_ptr); // O(bucket_count)

<HIVE_NAME>_handle <HIVE_NAME>_iter_to_handle(<HIVE_NAME>_iter _it);
<HIVE_NAME>_iter   <HIVE_NAME>_handle_del(<HIVE_NAME> *_hive, <HIVE_NAME>_handle _handle);
<HIVE_NAME>_handle <HIVE_NAME>_ptr_to_handle(<HIVE_NAME> *_hive, <HIVE_TYPE> *_ptr); // O(bucket_count)
<HIVE_NAME>_iter   <HIVE_NAME>_handle_to_iter(<HIVE_NAME>_handle _handle);
```

## Iterators and Handles
The only difference is that `_handle` is smaller, but can't be used for iteration.

## How to include
You can do it like in the example for simple use cases.

But if you want the container to be declared in a header file and implemented in a corresponding c file, you can do:

```C
// ab.h
#ifndef AB_H
#define AB_H

typedef struct A
{
    int i;
} A;

typedef struct B
{
    int j;
} B;

#define HIVE_TYPE A
#define HIVE_NAME AHive
#include "hive.h"

#define HIVE_TYPE B
#define HIVE_NAME BHive
#include "hive.h"

#endif
```

Now make a corresponding `.c` file for the implementation

```C
// ab.c
#define HIVE_IMPL_ALL
#include "ab.h"
```
