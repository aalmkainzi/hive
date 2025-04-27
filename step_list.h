#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>

#if !defined(TYPE) || !defined(NAME)

    #error "TYPE and NAME must be defined"

#endif

#if !defined(BUCKET_SIZE)

    #define BUCKET_SIZE 254

#endif

#if BUCKET_SIZE < 255

    #define sl_u uint8_t
    #define SL_U_MAX ((uint8_t)-1)

#else

    #define sl_u uint16_t
    #define SL_U_MAX ((uint16_t)-1)

#endif

#define CAT_(a,b) a##b
#define CAT(a,b) CAT_(a,b)

/*
IDEAS:
    - offset memory between buckets instead of next pointer (we have to keep track of the last elm then, shouldn't be a problem)
    - each new bucket is double the size of prev
*/

#define sl_bucket_t             CAT(NAME, _bucket)
#define sl_init                 CAT(NAME, _init)
#define sl_put                  CAT(NAME, _push)
#define sl_pop                  CAT(NAME, _pop)
#define sl_loop                 CAT(NAME, _loop)
#define sl_bucket_init          CAT(NAME, _bucket_init)
#define sl_bucket_put           CAT(NAME, _bucket_put)
#define sl_bucket_pop           CAT(NAME, _bucket_pop)
#define sl_bucket_is_elm_within CAT(NAME, _bucket_is_elm_within)

typedef int TYPE;

typedef struct sl_bucket_t
{
    sl_u first_elm_idx;
    sl_u jump_list[BUCKET_SIZE];
    typeof(TYPE) elms[BUCKET_SIZE];
    sl_bucket_t *next;
} sl_bucket_t;

typedef struct NAME
{
    sl_bucket_t *buckets;
    sl_bucket_t *tail;
    size_t count;
    size_t bucket_count;
} NAME;

void sl_init(NAME *sl)
{
    (*sl) = (NAME){
        .buckets      = NULL,
        .tail         = NULL,
        .count        = 0,
        .bucket_count = 0,
    };
}

TYPE *sl_bucket_put(sl_bucket_t *bucket, TYPE newElm)
{
    int emptyIndex = 0;
    for( ; emptyIndex < BUCKET_SIZE ; emptyIndex++)
    {
        if(bucket->jump_list[emptyIndex] != 0)
            goto emptyIndexFound;
    }
    return NULL;
    
    emptyIndexFound:
    bucket->elms[emptyIndex] = newElm;
    
    bucket->jump_list[emptyIndex] = 0;
    
    if(emptyIndex < bucket->first_elm_idx)
    {
        bucket->first_elm_idx = emptyIndex;
    }
    
    return &bucket->elms[emptyIndex];
}

bool sl_bucket_is_elm_within(sl_bucket_t *bucket, TYPE *elm)
{
    uintptr_t
    ibegin = (uintptr_t) (bucket->elms),
    iend   = (uintptr_t) (bucket->elms + BUCKET_SIZE),
    ielm   = (uintptr_t) (elm);
    return ielm >= ibegin && ielm < iend;
}

void sl_bucket_pop(sl_bucket_t *bucket, TYPE *elm)
{
    sl_u index = elm - bucket->elms;
    if(bucket->jump_list[index] != 0)
        return;
    
    if(index != BUCKET_SIZE - 1)
        bucket->jump_list[index] = bucket->jump_list[index + 1] + 1;
    else
        bucket->jump_list[index] = 1;
    
    for(int i = index - 1 ; i >= 0 && bucket->jump_list[i] != 0 ; i--)
    {
        bucket->jump_list[i] = bucket->jump_list[index] + (index - i);
    }
}

void sl_bucket_init(sl_bucket_t *bucket)
{
    bucket->first_elm_idx = SL_U_MAX;
    bucket->next = NULL;
    
    for(sl_u i = 0 ; i < BUCKET_SIZE ; i++)
    {
        bucket->jump_list[i] = BUCKET_SIZE - i;
    }
}

TYPE *sl_put(NAME *sl, TYPE newElm)
{
    TYPE *elmAdded = NULL;
    
    for(sl_bucket_t *bucket = sl->buckets ; bucket != NULL ; bucket = bucket->next)
    {
        elmAdded = sl_bucket_put(bucket, newElm);
        if(elmAdded != NULL)
        {
            return elmAdded;
        }
    }
    
    sl_bucket_t *new_bucket = (sl_bucket_t*) malloc(sizeof(sl_bucket_t));
    sl_bucket_init(new_bucket);
    
    if(sl->count > 0)
        sl->tail->next = new_bucket;
    else
        sl->buckets = new_bucket;
    
    sl->tail = new_bucket;
    elmAdded = sl_bucket_put(new_bucket, newElm);
    sl->count += 1;
    return elmAdded;
}

void sl_pop(NAME *sl, TYPE *elm)
{
    for(sl_bucket_t *bucket = sl->buckets ; bucket != NULL ; bucket = bucket->next)
    {
        if(sl_bucket_is_elm_within(bucket, elm))
        {
            sl_bucket_pop(bucket, elm);
            sl->count -= 1;
            return;
        }
    }
}

void sl_loop(NAME *sl, void(*f)(TYPE*,void*), void *arg)
{
    for(sl_bucket_t *bucket = sl->buckets ; bucket != NULL ; bucket = bucket->next)
    {
        for(sl_u i = bucket->first_elm_idx ; i < BUCKET_SIZE ; i++)
        {
            f(&bucket->elms[i], arg);
            i += bucket->jump_list[i];
        }
    }
}
