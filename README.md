# stable_pool
A container that provides pointer stability and faster iteration speed than linked list. Insertion order is not perserved.
## Example

```C
int main()
{
    int_sp ints;
    int_sp_init(&ints);
    
    int *one   = int_sp_put(&ints, 1);
    int *two   = int_sp_put(&ints, 2);
    int *three = int_sp_put(&ints, 3);
    int *four  = int_sp_put(&ints, 4);
    int *five  = int_sp_put(&ints, 5);
    
    // The library offers an iterator API
    int_sp_iter_t end = int_sp_end(&ints);
    for(int_sp_iter_t it = int_sp_begin(&ints) ; !int_sp_iter_eq(it, end) ; it = int_sp_iter_next(it))
    {
        printf("%d\n", *int_sp_iter_elm(it));
    }
    
    // And a foreach function
    int_sp_foreach(&ints, printInt, NULL);
    
    int_sp_pop(&ints, one);
    int_sp_pop(&ints, two);
    int_sp_pop(&ints, three);
    int_sp_pop(&ints, four);
    int_sp_pop(&ints, five);
    
    int_sp_deinit(&ints);
}
```
