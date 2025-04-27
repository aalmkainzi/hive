#include <stddef.h>

#if !defined(TYPE) || !defined(NAME)

    #error "TYPE and NAME must be defined"

#endif

#if !defined(BUCKET_SIZE)

    #define BUCKET_SIZE 512

#endif

#define CAT_(a,b) a##b
#define CAT(a,b) CAT_(a,b)

#define sl_bucket_t CAT(NAME, _bucket)
#define sl_init     CAT(init_, NAME)
#define sl_put      CAT(push_, NAME)
#define sl_pop      CAT(pop_,  NAME)
#define sl_loop     CAT(loop_, NAME)

typedef struct sl_bucket_t
{
    int first_elm_idx;
    int jump_list[BUCKET_SIZE];
    typeof(TYPE) buffer[BUCKET_SIZE];
    sl_bucket_t *next;
} sl_bucket_t;

typedef struct NAME
{
    CAT(NAME, _bucket) *buckets;
    size_t count;
} NAME;


void sl_init(NAME *sl)
{
    sl->buckets = NULL;
    sl->count   = 0;
}

TYPE *sl_put(NAME *sl, TYPE elm)
{
    
}

void sl_pop(NAME *sl, TYPE *elm)
{
    
}
