#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define SL_TYPE int

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

#undef sl_u
#undef SL_U_MAX
#define sl_u uintptr_t
#define SL_U_MAX UINTPTR_MAX

#define SL_CAT_(a,b) a##b
#define SL_CAT(a,b) SL_CAT_(a,b)

/*
IDEAS:
    - offset memory between buckets instead of next pointer (we have to keep track of the last elm then, shouldn't be a problem. Only caveat is that it's not exactly standard compliant to do that)
    - each new bucket is double the size of the prev
*/
/*
plan:
    - make sure to delete empty nodes
    - make the last elm jump of each bucket point to the next non empty elm in the next bucket (by using intptr_t)
    - the loop will then be like `for(T *curElm = sl->buckets->elms + sl->buckets->first_elm_idx ; curElm != sl->last_elm ; i += jump_list[i] )
        {body
        i++;}
*/

#define sl_bucket_t SL_CAT(SL_NAME, _bucket_t)
#define sl_init     SL_CAT(SL_NAME, _init)
#define sl_put      SL_CAT(SL_NAME, _put)
#define sl_pop      SL_CAT(SL_NAME, _pop)
#define sl_loop     SL_CAT(SL_NAME, _loop)
#define sl_deinit   SL_CAT(SL_NAME, _deinit)

#if !defined(SL_ONLY_DECL)
    #define sl_bucket_init          SL_CAT(SL_NAME, _bucket_init)
    #define sl_bucket_put           SL_CAT(SL_NAME, _bucket_put)
    #define sl_bucket_pop           SL_CAT(SL_NAME, _bucket_pop)
    #define sl_bucket_is_elm_within SL_CAT(SL_NAME, _bucket_is_elm_within)
    #define sl_bucket_first_elm     SL_CAT(SL_NAME, _bucket_first_elm)
    #define sl_bucket_last_elm      SL_CAT(SL_NAME, _bucket_last_elm)
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
    SL_TYPE *last_elm;
    size_t count;
    size_t bucket_count;
} SL_NAME;

void sl_init(SL_NAME *sl);
SL_TYPE *sl_put(SL_NAME *sl, SL_TYPE new_elm);
void sl_pop(SL_NAME *sl, SL_TYPE *elm);
void sl_loop(const SL_NAME *sl, void(*f)(SL_TYPE*,void*), void *arg);
void sl_deinit(SL_NAME *sl);

#if !defined(SL_ONLY_DECL)

void sl_bucket_init(sl_bucket_t *bucket);
SL_TYPE *sl_bucket_put(sl_bucket_t *bucket, SL_TYPE new_elm);
bool sl_bucket_pop(SL_NAME *sl, sl_bucket_t *bucket, SL_TYPE *elm);
bool sl_bucket_is_elm_within(const sl_bucket_t *bucket, const SL_TYPE *elm);
sl_u sl_bucket_first_elm(sl_bucket_t *bucket);
sl_u sl_bucket_last_elm(sl_bucket_t *bucket);

void sl_init(SL_NAME *sl)
{
    (*sl) = (SL_NAME){
        .buckets      = NULL,
        .tail         = NULL,
        .count        = 0,
        .bucket_count = 0,
        .last_elm     = NULL
    };
}

SL_TYPE *sl_bucket_put(sl_bucket_t *bucket, SL_TYPE new_elm)
{
    int emptyIndex = 0;
    for( ; emptyIndex < SL_BUCKET_SIZE ; emptyIndex++)
    {
        if(bucket->jump_list[emptyIndex] != 0)
            goto emptyIndexFound;
    }
    return NULL;
    
    emptyIndexFound:
    bucket->elms[emptyIndex] = new_elm;
    
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


// Things we want:
// after a pop, check if we just popped the last_elm
// if so, find and set it again
// DONE
bool sl_bucket_pop(SL_NAME *sl, sl_bucket_t *bucket, SL_TYPE *elm)
{
    sl_u index = elm - bucket->elms;
    if(bucket->jump_list[index] != 0)
        return false;
    
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
                    goto check_if_bucket_empty;
                }
            }
            bucket->first_elm_idx = SL_U_MAX;
        }
    }
    
    check_if_bucket_empty:
    
    bool is_empty = true;
    sl_u last_elm_in_bucket;
    for(last_elm_in_bucket = 0 ; last_elm_in_bucket < SL_BUCKET_SIZE ; last_elm_in_bucket++)
    {
        if(bucket->jump_list[last_elm_in_bucket] == 0)
        {
            is_empty = false;
            break;
        }
    }
    
    if(sl->last_elm == &bucket->elms[index])
    {
        if(is_empty)
        {
            if(sl->bucket_count == 1)
            {
                sl->last_elm = NULL;
            }
            else
            {
                // look in the previous bucket
                sl_bucket_t *current = sl->buckets;
                while(current->next != sl->tail)
                {
                    current = current->next;
                }
                
                sl_u last_elm_idx = sl_bucket_last_elm(current);
                sl->last_elm = &current->elms[last_elm_idx];
            }
        }
        else
        {
            // look in current bucket
            sl->last_elm = &bucket->elms[last_elm_in_bucket];
        }
    }
    
    return is_empty;
}

sl_u sl_bucket_first_elm(sl_bucket_t *bucket)
{
    for(sl_u i = 0 ; i < SL_BUCKET_SIZE ; i--)
    {
        if(bucket->jump_list[i] == 0)
            return i;
    }
    return (sl_u)-1;
}

sl_u sl_bucket_last_elm(sl_bucket_t *bucket)
{
    for(sl_u i = SL_BUCKET_SIZE - 2 ; i >= 0 ; i--)
    {
        if(bucket->jump_list[i] == 0)
            return i;
    }
    return (sl_u)-1;
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

SL_TYPE *sl_put(SL_NAME *sl, SL_TYPE new_elm)
{
    SL_TYPE *elm_added = NULL;
    
    for(sl_bucket_t *bucket = sl->buckets ; bucket != NULL ; bucket = bucket->next)
    {
        elm_added = sl_bucket_put(bucket, new_elm);
        if(elm_added != NULL)
        {
            sl->count += 1;
            sl_u index = elm_added - bucket->elms;
            
            sl_u last_elm_in_bucket;
            for(last_elm_in_bucket = SL_BUCKET_SIZE - 2 ; last_elm_in_bucket >= 0 ; last_elm_in_bucket--)
            {
                if(bucket->jump_list[last_elm_in_bucket] == 0)
                    break;
            }
            
            if(bucket == sl->tail)
            {
                sl->last_elm = &bucket->elms[last_elm_in_bucket];
            }
            else
            {
                // TODO make the jump_list[i] go to the next bucket if it is elm_added
                if(elm_added - bucket->elms == last_elm_in_bucket)
                {
                    sl_bucket_t *next_bucket = bucket->next;
                    
                }
            }
            
            return elm_added;
        }
    }
    
    sl_bucket_t *new_bucket = (sl_bucket_t*) malloc(sizeof(sl_bucket_t));
    sl_bucket_init(new_bucket);
    
    if(sl->count > 0)
        sl->tail->next = new_bucket;
    else
        sl->buckets = new_bucket;
    
    sl->bucket_count += 1;
    sl->tail = new_bucket;
    elm_added = sl_bucket_put(new_bucket, new_elm);
    sl->count += 1;
    sl->last_elm = elm_added;
    return elm_added;
}

void sl_pop(SL_NAME *sl, SL_TYPE *elm)
{
    sl_bucket_t *prev = NULL;
    for(sl_bucket_t *bucket = sl->buckets ; bucket != NULL ; bucket = bucket->next)
    {
        if(sl_bucket_is_elm_within(bucket, elm))
        {
            if(sl_bucket_pop(sl, bucket, elm))
            {
                if(prev != NULL)
                {
                    prev->next = bucket->next;
                }
                else
                {
                    sl->buckets = bucket->next;
                }
                if(bucket == sl->tail)
                    sl->tail = prev;
                free(bucket);
                sl->bucket_count -= 1;
            }
            sl->count -= 1;
            return;
        }
        prev = bucket;
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
#undef sl_bucket_first_elm
#undef sl_bucket_last_elm

#undef SL_CAT_
#undef SL_CAT
