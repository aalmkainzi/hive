# hive
A container that provides pointer/iterator stability and faster iteration speed than linked list. Insertion order is not perserved.
## Example

```C
#define SP_IMPL
#define SP_TYPE int
#define SP_NAME int_sp
#include "stable_pool.h"

int main()
{
    int_sp ints;
    int_sp_init(&ints);
    
    int *one   = int_sp_put(&ints, 1);
    int *two   = int_sp_put(&ints, 2);
    int *three = int_sp_put(&ints, 3);
    int *four  = int_sp_put(&ints, 4);
    int *five  = int_sp_put(&ints, 5);
    
    for(int_sp_iter_t it = int_sp_begin(&ints) ; !int_sp_iter_is_end(it) ; int_sp_iter_go_next(&it))
    {
        printf("%d\n", *int_sp_iter_elm(it));
    }
    
    int_sp_pop(&ints, one);
    int_sp_pop(&ints, two);
    int_sp_pop(&ints, three);
    int_sp_pop(&ints, four);
    int_sp_pop(&ints, five);
    
    int_sp_deinit(&ints);
}
```
