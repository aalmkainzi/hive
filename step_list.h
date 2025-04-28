#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#if !defined(SL_TYPE) || !defined(SL_NAME)

    #error "SL_TYPE and SL_NAME must be defined"

#endif

#if !defined(SL_BUCKET_SIZE)

    #define SL_BUCKET_SIZE 254

#endif

#if SL_BUCKET_SIZE < 255

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
    - offset memory between buckets instead of next pointer (we have to keep track of the last elm then, shouldn't be a problem. Only caveat is that it's not exactly standard compliant to do that)
    - each new bucket is double the size of the prev
*/

#define sl_bucket_t             CAT(SL_NAME, _bucket_t)
#define sl_init                 CAT(SL_NAME, _init)
#define sl_put                  CAT(SL_NAME, _put)
#define sl_pop                  CAT(SL_NAME, _pop)
#define sl_loop                 CAT(SL_NAME, _loop)
#define sl_deinit               CAT(SL_NAME, _deinit)

#if !defined(SL_ONLY_DECL)
    #define sl_bucket_init          CAT(SL_NAME, _bucket_init)
    #define sl_bucket_put           CAT(SL_NAME, _bucket_put)
    #define sl_bucket_pop           CAT(SL_NAME, _bucket_pop)
    #define sl_bucket_is_elm_within CAT(SL_NAME, _bucket_is_elm_within)
#endif

#define SL_FOREACH(sl, body) \
for(typeof((sl)->buckets) sl_it_bucket = (sl)->buckets ; sl_it_bucket != NULL ; sl_it_bucket = sl_it_bucket->next) \
for(sl_u sl_it_elms = sl_it_bucket->first_elm_idx ; sl_it_elms < (sizeof(sl_it_bucket->elms) / sizeof(*(sl_it_bucket->elms))) - 1 ; sl_it_elms += sl_it_bucket->jump_list[sl_it_elms]) \
do { \
    body \
    sl_it_elms++; \
} while(0)

#define SL_IT \
((int*const) &sl_it_bucket->elms[sl_it_elms])

typedef struct sl_bucket_t
{
    sl_u first_elm_idx;
    sl_u jump_list[SL_BUCKET_SIZE + 1];
    typeof(SL_TYPE) elms[SL_BUCKET_SIZE + 1];
    struct sl_bucket_t *next;
} sl_bucket_t;

typedef struct SL_NAME
{
    sl_bucket_t *buckets;
    sl_bucket_t *tail;
    size_t count;
    size_t bucket_count;
} SL_NAME;

#if defined(SL_ONLY_DECL)

void sl_init(SL_NAME *sl);
SL_TYPE *sl_put(SL_NAME *sl, SL_TYPE newElm);
void sl_pop(SL_NAME *sl, SL_TYPE *elm);
void sl_loop(const SL_NAME *sl, void(*f)(SL_TYPE*,void*), void *arg);
void sl_deinit(SL_NAME *sl);

#else

void sl_init(SL_NAME *sl)
{
    (*sl) = (SL_NAME){
        .buckets      = NULL,
        .tail         = NULL,
        .count        = 0,
        .bucket_count = 0,
    };
}

SL_TYPE *sl_bucket_put(sl_bucket_t *bucket, SL_TYPE newElm)
{
    int emptyIndex = 0;
    for( ; emptyIndex < SL_BUCKET_SIZE ; emptyIndex++)
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

bool sl_bucket_is_elm_within(const sl_bucket_t *bucket, const SL_TYPE *elm)
{
    uintptr_t
    ibegin = (uintptr_t) (bucket->elms),
    iend   = (uintptr_t) (bucket->elms + SL_BUCKET_SIZE),
    ielm   = (uintptr_t) (elm);
    return ielm >= ibegin && ielm < iend;
}

void sl_bucket_pop(sl_bucket_t *bucket, SL_TYPE *elm)
{
    sl_u index = elm - bucket->elms;
    if(bucket->jump_list[index] != 0)
        return;
    
    if(index != SL_BUCKET_SIZE - 1)
        bucket->jump_list[index] = bucket->jump_list[index + 1] + 1;
    else
        bucket->jump_list[index] = 1;
    
    int i;
    for(i = index - 1 ; i >= 0 && bucket->jump_list[i] != 0 ; i--)
    {
        bucket->jump_list[i] = bucket->jump_list[index] + (index - i);
    }
    
    if(index == bucket->first_elm_idx)
    {
        if(i >= 0)
        {
            bucket->first_elm_idx = i;
        }
        else
        {
            for(int j = index + 1 ; j < SL_BUCKET_SIZE ; j++)
            {
                if(bucket->jump_list[j] == 0)
                {
                    bucket->first_elm_idx = j;
                    return;
                }
            }
            bucket->first_elm_idx = SL_U_MAX;
        }
    }
}

void sl_bucket_init(sl_bucket_t *bucket)
{
    bucket->first_elm_idx = SL_U_MAX;
    bucket->next = NULL;
    
    for(sl_u i = 0 ; i < SL_BUCKET_SIZE ; i++)
    {
        bucket->jump_list[i] = SL_BUCKET_SIZE - i;
    }
    bucket->jump_list[SL_BUCKET_SIZE] = 1;
}

SL_TYPE *sl_put(SL_NAME *sl, SL_TYPE newElm)
{
    SL_TYPE *elmAdded = NULL;
    
    for(sl_bucket_t *bucket = sl->buckets ; bucket != NULL ; bucket = bucket->next)
    {
        elmAdded = sl_bucket_put(bucket, newElm);
        if(elmAdded != NULL)
        {
            sl->count += 1;
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

void sl_pop(SL_NAME *sl, SL_TYPE *elm)
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

void sl_loop(const SL_NAME *sl, void(*f)(SL_TYPE*,void*), void *arg)
{
    for(sl_bucket_t *bucket = sl->buckets ; bucket != NULL ; bucket = bucket->next)
    {
        for(sl_u i = bucket->first_elm_idx ; i < SL_BUCKET_SIZE ; i += bucket->jump_list[i])
        {
            f(&bucket->elms[i], arg);
            i++;
        }
    }
}

void sl_deinit(SL_NAME *sl)
{
    for(sl_bucket_t *current = sl->buckets ; current != NULL ; )
    {
        sl_bucket_t *next = current->next;
        free(current);
        current = next;
    }
}

#endif

#undef SL_BUCKET_SIZE
#undef SL_TYPE
#undef SL_NAME

#undef sl_bucket_t
#undef sl_init
#undef sl_put
#undef sl_pop
#undef sl_loop
#undef sl_bucket_init
#undef sl_bucket_put
#undef sl_bucket_pop
#undef sl_bucket_is_elm_within
#undef CAT_
#undef CAT
