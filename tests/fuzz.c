// NOT DONE

#include <stddef.h>
#include <stdint.h>
#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"

#define SP_TYPE unsigned int
#define SP_NAME ints
#define SP_IMPL
#include "stable_pool.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    srand(69420);
    ints n;
    ints_init(&n);
    
    unsigned int **cpyptr = NULL;
    unsigned int *cpy = NULL;
    
    for(size_t i = 0 ; i < size ; i++)
    {
        switch(data[i] % 6)
        {
            case 0:
            {
                unsigned int val = rand();
                unsigned int *elm = ints_put(&n, val);
                arrput(cpy, val);
                arrput(cpyptr, elm);
                break;
            }
            case 1:
            {
                
            }
        }
    }
    return 0;
}
