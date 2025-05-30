# hive
A container that provides pointer/iterator stability and fast iteration. Insertion order is not preserved.
## Example

```C
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
    my_hive_iter_t thirty = my_hive_put(&ints, 30);
    my_hive_put(&ints, 40);
    my_hive_put(&ints, 50);
    
    for(my_hive_iter_t it = my_hive_begin(&ints) ; !my_hive_iter_eq(it, my_hive_end(&ints)) ; my_hive_iter_go_next(&it))
    {
        printf("%d\n", *my_hive_iter_elm(it));
    }
    
    my_hive_iter_del(&ints, thirty);
    
    my_hive_deinit(&ints);
}
```

## API
```C
void             HIVE_NAME_init(HIVE_NAME *_hv);
HIVE_NAME        HIVE_NAME_clone(const HIVE_NAME *const _hv);
HIVE_NAME_iter_t HIVE_NAME_put(HIVE_NAME *hv, HIVE_TYPE _new_elm);
void             HIVE_NAME_put_all(HIVE_NAME *_hv, const HIVE_TYPE *_elms, size_t _nelms);
HIVE_NAME_iter_t HIVE_NAME_del(HIVE_NAME *_hv, HIVE_TYPE *_elm);
void             HIVE_NAME_deinit(HIVE_NAME *_hv);

HIVE_NAME_iter_t HIVE_NAME_begin(const HIVE_NAME *_hv);
HIVE_NAME_iter_t HIVE_NAME_end(const HIVE_NAME *_hv);
HIVE_NAME_iter_t HIVE_NAME_iter_next(HIVE_NAME_iter_t _it);
void             HIVE_NAME_iter_go_next(HIVE_NAME_iter_t *_it);
HIVE_TYPE *      HIVE_NAME_iter_elm(HIVE_NAME_iter_t _it);
HIVE_NAME_iter_t HIVE_NAME_iter_del(HIVE_NAME *_hv, HIVE_NAME_iter_t _it);
bool             HIVE_NAME_iter_eq(HIVE_NAME_iter_t _a, HIVE_NAME_iter_t _b);
```

## How to include
Make a new header for your hive type. Inside it, define `HIVE_TYPE` and `HIVE_NAME` and include the `"hive.h"` header
e.g.

```C
// foo_hive.h
#ifndef FOO_HIVE_H
#define FOO_HIVE_H

#define HIVE_TYPE Foo
#define HIVE_NAME Foo_Hive
#include "hive.h"

#endif
```

And make a `.c` file to include the implementation

```C
// foo_hive.c
#define HIVE_TYPE Foo
#define HIVE_NAME Foo_Hive
#define HIVE_IMPL
#include "hive.h"
```
