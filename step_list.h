#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#if !defined(SL_TYPE) || !defined(SL_NAME)

    #error "SL_TYPE and SL_NAME must be defined"

#endif

#if !defined(SL_BUCKET_SIZE)

    #define SL_BUCKET_SIZE (511)

#endif

#define sl_index_t   int64_t
#define SL_INDEX_MAX INT64_MAX

#define SL_CAT_(a,b) a##b
#define SL_CAT(a,b) SL_CAT_(a,b)

#define sl_iter_t               SL_CAT(SL_NAME, _iter_t)
#define sl_bucket_t             SL_CAT(SL_NAME, _bucket_t)
#define sl_entry_t              SL_CAT(SL_NAME, _entry_t)
#define sl_next_ptr_entry_t       SL_CAT(SL_NAME, _next_ptr_entry_t)
#define sl_init                 SL_CAT(SL_NAME, _init)
#define sl_put                  SL_CAT(SL_NAME, _put)
#define sl_pop                  SL_CAT(SL_NAME, _pop)
#define sl_foreach              SL_CAT(SL_NAME, _foreach)
#define sl_begin                SL_CAT(SL_NAME, _begin)
#define sl_end                  SL_CAT(SL_NAME, _end)
#define sl_deinit               SL_CAT(SL_NAME, _deinit)

#define sl_bucket_init          SL_CAT(SL_NAME, _bucket_init)
#define sl_bucket_put           SL_CAT(SL_NAME, _bucket_put)
#define sl_bucket_pop           SL_CAT(SL_NAME, _bucket_pop)
#define sl_bucket_is_elm_within SL_CAT(SL_NAME, _bucket_is_elm_within)
#define sl_bucket_first_elm     SL_CAT(SL_NAME, _bucket_first_elm)
#define sl_bucket_last_elm      SL_CAT(SL_NAME, _bucket_last_elm)
#define sl_bucket_prev          SL_CAT(SL_NAME, _bucket_prev)

#define sl_iter_next            SL_CAT(SL_NAME, _iter_next)
#define sl_iter_elm             SL_CAT(SL_NAME, _iter_elm)
#define sl_iter_pop             SL_CAT(SL_NAME, _iter_pop)
#define sl_iter_eq              SL_CAT(SL_NAME, _iter_eq)

#define SL_ARR_LEN(arr) \
sizeof(arr) / sizeof(arr[0])

#define SL_FOREACH(sl, body)                                                                                              \
do                                                                                                                        \
{                                                                                                                         \
    typeof(sl) sl_sl = sl;                                                                                                \
    typeof(sl_sl->buckets) sl_bucket = sl_sl->buckets;                                                                    \
    if(sl_bucket)                                                                                                         \
    {                                                                                                                     \
        typeof(&sl_sl->buckets->elms[0]) sl_entryp = &sl_bucket->elms[sl_bucket->first_elm_idx];                          \
        typeof(&sl_sl->buckets->next_ptrs[0]) sl_next_ptrp = &sl_bucket->next_ptrs[sl_bucket->first_elm_idx];                   \
        typeof(&sl_sl->buckets->next_ptrs[0]) sl_lastentry = &sl_sl->tail->next_ptrs[ SL_ARR_LEN(sl_sl->buckets->elms) - 1 ]; \
        for(                                                                                                              \
            ;                                                                                                             \
            sl_next_ptrp != sl_lastentry ;                                                                                  \
            ++sl_next_ptrp,                                                                                                 \
            ++sl_entryp, sl_entryp = sl_next_ptrp->next_entry,                                                              \
            sl_next_ptrp = sl_next_ptrp->next                                                                                 \
        )                                                                                                                 \
        {                                                                                                                 \
            body                                                                                                          \
        }                                                                                                                 \
    }                                                                                                                     \
} while(0)

#define SL_IT \
((typeof(sl_entryp->value)*const) &sl_entryp->value)

typedef struct sl_entry_t
{
    typeof(SL_TYPE) value;
} sl_entry_t;

typedef struct sl_next_ptr_entry_t
{
    sl_entry_t *next_entry;
    struct sl_next_ptr_entry_t *next;
} sl_next_ptr_entry_t;

typedef struct sl_bucket_t
{
    sl_index_t first_elm_idx;
    sl_entry_t elms[SL_BUCKET_SIZE + 1];
    sl_next_ptr_entry_t next_ptrs[SL_BUCKET_SIZE + 1];
    struct sl_bucket_t *next;
} sl_bucket_t;

typedef struct SL_NAME
{
    sl_bucket_t *buckets;
    sl_bucket_t *tail;
    sl_entry_t *last_elm;
    size_t count;
    size_t bucket_count;
} SL_NAME;

typedef struct sl_iter_t
{
    SL_NAME *sl;
    sl_entry_t *elm_entry;
    sl_next_ptr_entry_t *next_ptr_entry;
} sl_iter_t;

void sl_init(SL_NAME *sl);
SL_TYPE *sl_put(SL_NAME *sl, SL_TYPE new_elm);
sl_iter_t sl_pop(SL_NAME *sl, SL_TYPE *elm);
void sl_foreach(const SL_NAME *sl, void(*f)(SL_TYPE*,void*), void *arg);
void sl_deinit(SL_NAME *sl);

sl_iter_t sl_begin(SL_NAME *sl);
sl_iter_t sl_end(SL_NAME *sl);
sl_iter_t sl_iter_next(sl_iter_t it);
SL_TYPE *sl_iter_elm(sl_iter_t it);
sl_iter_t sl_iter_pop(sl_iter_t it);
bool sl_iter_eq(sl_iter_t a, sl_iter_t b);

#define SL_IMPL
#if defined(SL_IMPL)

void sl_bucket_init(sl_bucket_t *bucket);
SL_TYPE *sl_bucket_put(SL_NAME *sl, sl_bucket_t *bucket, SL_TYPE new_elm);
bool sl_bucket_pop(SL_NAME *sl, sl_bucket_t *bucket, SL_TYPE *elm);
bool sl_bucket_is_elm_within(const sl_bucket_t *bucket, const SL_TYPE *elm);
sl_index_t sl_bucket_first_elm(sl_bucket_t *bucket);
sl_index_t sl_bucket_last_elm(sl_bucket_t *bucket);
sl_bucket_t *sl_bucket_prev(SL_NAME *sl, sl_bucket_t *bucket);

void sl_init(SL_NAME *sl)
{
    (*sl) = (SL_NAME){
        .buckets      = NULL,
        .tail         = NULL,
        .last_elm     = NULL,
        .count        = 0,
        .bucket_count = 0
    };
}

SL_TYPE *sl_put(SL_NAME *sl, SL_TYPE new_elm)
{
    SL_TYPE *elm_added = NULL;
    
    for(sl_bucket_t *bucket = sl->buckets ; bucket != NULL ; bucket = bucket->next)
    {
        elm_added = sl_bucket_put(sl, bucket, new_elm);
        if(elm_added != NULL)
        {
            sl->count += 1;
            sl_index_t last_elm_in_bucket = sl_bucket_last_elm(bucket);
            
            if(bucket == sl->tail)
            {
                sl->last_elm = &bucket->elms[last_elm_in_bucket];
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
    elm_added = sl_bucket_put(sl, new_bucket, new_elm);
    sl->count += 1;
    sl->last_elm = (sl_entry_t*) elm_added;
    return elm_added;
}

sl_iter_t sl_pop(SL_NAME *sl, SL_TYPE *elm)
{
    sl_bucket_t *prev = NULL;
    sl_iter_t ret = {
        .sl = sl
    };
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
                {
                    sl->tail = prev;
                    sl->last_elm = (sl_entry_t*) sl_bucket_last_elm(sl->tail);
                }
                
                if(bucket->next != NULL)
                {
                    sl_index_t first_index_of_next_bucket = sl_bucket_first_elm(bucket->next);
                    ret.next_ptr_entry = &bucket->next->next_ptrs[first_index_of_next_bucket];
                    ret.elm_entry = &bucket->next->elms[first_index_of_next_bucket];
                }
                else
                {
                    ret.next_ptr_entry = &sl->tail->next_ptrs[SL_BUCKET_SIZE];
                    ret.elm_entry = &sl->tail->elms[SL_BUCKET_SIZE];
                }
                free(bucket);
                sl->bucket_count -= 1;
            }
            sl->count -= 1;
            return ret;
        }
        prev = bucket;
    }
    return ret;
}

void sl_foreach(const SL_NAME *sl, void(*f)(SL_TYPE*,void*), void *arg)
{
    sl_bucket_t *bucket = sl->buckets;
    sl_entry_t *entryp = &bucket->elms[bucket->first_elm_idx];
    sl_next_ptr_entry_t *next_ptrp = &bucket->next_ptrs[bucket->first_elm_idx];
    sl_next_ptr_entry_t *lastentry = &sl->tail->next_ptrs[SL_BUCKET_SIZE];
    for(
        ;
        next_ptrp != lastentry ;
        ++next_ptrp,
        ++entryp, entryp = next_ptrp->next_entry,
        next_ptrp = next_ptrp->next
        )
    {
        f(&entryp->value, arg);
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

void sl_bucket_init(sl_bucket_t *bucket)
{
    bucket->first_elm_idx = SL_INDEX_MAX;
    bucket->next = NULL;
    
    uintptr_t addrs_of_end_of_bucket_elm = (uintptr_t) &bucket->elms[SL_BUCKET_SIZE];
    uintptr_t addrs_of_end_of_bucket_next_ptr = (uintptr_t) &bucket->next_ptrs[SL_BUCKET_SIZE];
    for(sl_index_t i = 0 ; i < SL_BUCKET_SIZE ; i++)
    {
        uintptr_t addrs_of_curr_elm = (uintptr_t) &bucket->elms[i];
        uintptr_t addrs_of_curr_next_ptr = (intptr_t) &bucket->next_ptrs[i];
        bucket->next_ptrs[i].next_entry = &bucket->elms[SL_BUCKET_SIZE];
        bucket->next_ptrs[i].next = &bucket->next_ptrs[SL_BUCKET_SIZE];
    }
    bucket->next_ptrs[SL_BUCKET_SIZE].next_entry = &bucket->elms[SL_BUCKET_SIZE];
    bucket->next_ptrs[SL_BUCKET_SIZE].next = &bucket->next_ptrs[SL_BUCKET_SIZE];
}

SL_TYPE *sl_bucket_put(SL_NAME *sl, sl_bucket_t *bucket, SL_TYPE new_elm)
{
    int emptyIndex = 0;
    for( ; emptyIndex < SL_BUCKET_SIZE ; emptyIndex++)
    {
        if(bucket->next_ptrs[emptyIndex].next_entry != &bucket->elms[emptyIndex])
            goto emptyIndexFound;
    }
    return NULL;
    
    emptyIndexFound:
    bucket->elms[emptyIndex].value = new_elm;
    
    bucket->next_ptrs[emptyIndex].next_entry = &bucket->elms[emptyIndex];
    bucket->next_ptrs[emptyIndex].next = &bucket->next_ptrs[emptyIndex];
    
    if(emptyIndex < bucket->first_elm_idx)
    {
        bucket->first_elm_idx = emptyIndex;
        uintptr_t addrs_of_first_elm_in_this_bucket = (uintptr_t) &bucket->elms[bucket->first_elm_idx];
        uintptr_t addrs_of_first_next_ptr_in_this_bucket = (uintptr_t) &bucket->next_ptrs[bucket->first_elm_idx];
        // TODO make all the prev empty elms in this bucket next_ptr to the new first elm
        for(sl_index_t i = 0 ; i < bucket->first_elm_idx ; i++)
        {
            uintptr_t addrs_of_curr_elm = (uintptr_t) &bucket->elms[i];
            uintptr_t addrs_of_curr_next_ptr = (uintptr_t) &bucket->next_ptrs[i];
            bucket->next_ptrs[i].next_entry = &bucket->elms[bucket->first_elm_idx];
            bucket->next_ptrs[i].next = &bucket->next_ptrs[bucket->first_elm_idx];
        }
        
        if(bucket != sl->buckets)
        {
            // we need to make the prev bucket link to this one's first elm
            sl_bucket_t *prev_bucket = sl_bucket_prev(sl, bucket);
            
            sl_index_t last_elm_in_prev_bucket = sl_bucket_last_elm(prev_bucket);
            uintptr_t addrs_of_first_elm_in_this_bucket = (uintptr_t) &bucket->elms[bucket->first_elm_idx];
            uintptr_t addrs_of_first_next_ptr_in_this_bucket = (uintptr_t) &bucket->next_ptrs[bucket->first_elm_idx];
            for(sl_index_t k = last_elm_in_prev_bucket + 1 ; k < SL_BUCKET_SIZE + 1 ; k++)
            {
                uintptr_t addrs_of_curr_elm    = (uintptr_t) &prev_bucket->elms[k];
                uintptr_t addrs_of_curr_next_ptr = (uintptr_t) &prev_bucket->next_ptrs[k];
                prev_bucket->next_ptrs[k].next_entry = &bucket->elms[bucket->first_elm_idx];
                prev_bucket->next_ptrs[k].next = &bucket->next_ptrs[bucket->first_elm_idx];
            }
        }
    }
    
    return &bucket->elms[emptyIndex].value;
}

bool sl_bucket_pop(SL_NAME *sl, sl_bucket_t *bucket, SL_TYPE *elm)
{
    sl_entry_t *as_entry = (sl_entry_t*) elm;
    ptrdiff_t index = as_entry - bucket->elms;
    if(bucket->next_ptrs[index].next != &bucket->next_ptrs[index])
        return false;
    
    bool is_first_elm = false;
    sl_index_t index_of_first_elm = sl_bucket_first_elm(bucket);
    if(index == index_of_first_elm)
    {
        is_first_elm = true;
    }
    
    ptrdiff_t next_elm;
    for(next_elm = index + 1 ; next_elm < SL_BUCKET_SIZE && bucket->next_ptrs[next_elm].next != &bucket->next_ptrs[next_elm] ; next_elm++ )
    {
        ;
    }
    
    if(next_elm < SL_BUCKET_SIZE)
    {
        bucket->next_ptrs[index].next_entry = &bucket->elms[next_elm];
        bucket->next_ptrs[index].next = &bucket->next_ptrs[next_elm];
    }
    else
    {
        sl_bucket_t *next = bucket->next;
        if(next != NULL)
        {
            sl_index_t first_elm_in_next_bucket = sl_bucket_first_elm(next);
            bucket->next_ptrs[index].next = &next->next_ptrs[first_elm_in_next_bucket];
            bucket->next_ptrs[index].next_entry = &next->elms[first_elm_in_next_bucket];
        }
        else
        {
            bucket->next_ptrs[index].next = &bucket->next_ptrs[SL_BUCKET_SIZE];
            bucket->next_ptrs[index].next_entry = &bucket->elms[SL_BUCKET_SIZE];
        }
    }
    
    for(ptrdiff_t i = (ptrdiff_t)index - 1 ; i >= 0 && bucket->next_ptrs[i].next != &bucket->next_ptrs[i] ; i--)
    {
        bucket->next_ptrs[i].next_entry = bucket->next_ptrs[index].next_entry;
        bucket->next_ptrs[i].next = bucket->next_ptrs[index].next;
    }
    
    bool is_empty = false;
    if(index == bucket->first_elm_idx)
    {
        for(sl_index_t j = index + 1 ; j < SL_BUCKET_SIZE ; j++)
        {
            if(bucket->next_ptrs[j].next == &bucket->next_ptrs[j])
            {
                bucket->first_elm_idx = j;
                goto not_empty;
            }
        }
        is_empty = true;
        bucket->first_elm_idx = SL_INDEX_MAX; // this doesn't really matter, this node will get deleted since it's empty
    }
    
    not_empty:
    
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
                while(current->next != bucket)
                {
                    current = current->next;
                }
                
                sl_index_t last_elm_idx = sl_bucket_last_elm(current);
                sl->last_elm = &current->elms[last_elm_idx];
            }
        }
        else
        {
            // look in current bucket
            
            int64_t last_elm_in_bucket;
            for(last_elm_in_bucket = index - 1 ; bucket->next_ptrs[last_elm_in_bucket].next != &bucket->next_ptrs[last_elm_in_bucket] ; last_elm_in_bucket--)
                ;
            
            sl->last_elm = &bucket->elms[last_elm_in_bucket];
        }
    }
    
    // if we just deleted the first elm
    // and current bucket is empty and is not the head
    // then go to the previous bucket and correct its next_ptrs
    if(is_first_elm && bucket != sl->buckets)
    {
        if(!is_empty)
        {
            // pick a new first_elm and make it link to the prev bucket
            
            sl_index_t first_elm_in_this_bucket = sl_bucket_first_elm(bucket);
            sl_bucket_t *prev_bucket = sl_bucket_prev(sl, bucket);
            
            sl_index_t last_elm_in_prev_bucket = sl_bucket_last_elm(prev_bucket);
            
            uintptr_t addrs_of_first_elm_in_this_bucket = (uintptr_t) &bucket->elms[first_elm_in_this_bucket];
            uintptr_t addrs_of_first_next_ptr_in_this_bucket = (uintptr_t) &bucket->next_ptrs[first_elm_in_this_bucket];
            for(sl_index_t k = last_elm_in_prev_bucket + 1 ; k < SL_BUCKET_SIZE + 1 ; k++)
            {
                uintptr_t addrs_of_curr_elm = (uintptr_t) &prev_bucket->elms[k];
                uintptr_t addrs_of_curr_next_ptr = (uintptr_t) &prev_bucket->next_ptrs[k];
                prev_bucket->next_ptrs[k].next_entry = &bucket->elms[first_elm_in_this_bucket];
                prev_bucket->next_ptrs[k].next = &bucket->next_ptrs[first_elm_in_this_bucket];
            }
        }
        else
        {
            // find the first elm in the next bucket, make the previous bucket next_ptr to it
            // if no next bucket, make the prev bucket point to its last dummy elm at [SL_BUCKET_SIZE]
            sl_bucket_t *next_bucket = bucket->next;
            
            if(next_bucket != NULL)
            {
                sl_bucket_t *prev_bucket = sl_bucket_prev(sl, bucket);
                sl_index_t first_elm_in_next_bucket = sl_bucket_first_elm(next_bucket);
                uintptr_t addrs_of_first_elm_in_next_bucket = (uintptr_t) &next_bucket->elms[first_elm_in_next_bucket];
                uintptr_t addrs_of_first_next_ptr_in_next_bucket = (uintptr_t) &next_bucket->next_ptrs[first_elm_in_next_bucket];
                for(sl_index_t i = SL_BUCKET_SIZE - 1 ; prev_bucket->next_ptrs[i].next != &prev_bucket->next_ptrs[i] ; i--)
                {
                    uintptr_t addrs_of_curr_elm  = (uintptr_t) &prev_bucket->elms[i];
                    uintptr_t addrs_of_curr_next_ptr = (uintptr_t) &prev_bucket->next_ptrs[i];
                    prev_bucket->next_ptrs[i].next_entry = &next_bucket->elms[first_elm_in_next_bucket];
                    prev_bucket->next_ptrs[i].next = &next_bucket->next_ptrs[first_elm_in_next_bucket];
                }
            }
            else
            {
                // handle tail case
                // make the prev bucket link with its last index
                sl_bucket_t *prev_bucket = sl_bucket_prev(sl, bucket);
                sl_index_t last_elm_in_prev_bucket = sl_bucket_last_elm(prev_bucket);
                
                // we set the next_ptr of the last elm to 1 for some reason
                // can't think of a better idea
                // maybe make it loop back to the first elm of the sl?
                prev_bucket->next_ptrs[SL_BUCKET_SIZE].next_entry = &prev_bucket->elms[SL_BUCKET_SIZE]; //sizeof(sl_entry_t);
                prev_bucket->next_ptrs[SL_BUCKET_SIZE].next = &prev_bucket->next_ptrs[SL_BUCKET_SIZE];
                uintptr_t addrs_of_end_of_bucket_elm = (uintptr_t) &prev_bucket->elms[SL_BUCKET_SIZE];
                uintptr_t addrs_of_end_of_bucket_next_ptr = (uintptr_t) &prev_bucket->next_ptrs[SL_BUCKET_SIZE];
                for(sl_index_t i = SL_BUCKET_SIZE - 1 ; i != last_elm_in_prev_bucket ; i--)
                {
                    uintptr_t addrs_of_curr_elm = (uintptr_t) &prev_bucket->elms[i];
                    uintptr_t addrs_of_curr_next_ptr = (uintptr_t) &prev_bucket->next_ptrs[i];
                    prev_bucket->next_ptrs[i].next_entry = &prev_bucket->elms[SL_BUCKET_SIZE];
                    prev_bucket->next_ptrs[i].next = &prev_bucket->next_ptrs[SL_BUCKET_SIZE];
                }
            }
        }
    }
    
    return is_empty;
}

sl_index_t sl_bucket_first_elm(sl_bucket_t *bucket)
{
    return bucket->first_elm_idx;
    // for(sl_index_t i = 0 ; i < SL_BUCKET_SIZE ; i++)
    // {
    //     if(bucket->next_ptrs[i].elm_next_ptr == 0)
    //         return i;
    // }
    // return SL_INDEX_MAX;
}

sl_index_t sl_bucket_last_elm(sl_bucket_t *bucket)
{
    for(sl_index_t i = SL_BUCKET_SIZE - 1 ; i >= 0 ; i--)
    {
        if(bucket->next_ptrs[i].next == &bucket->next_ptrs[i])
            return i;
    }
    return SL_INDEX_MAX;
}

sl_bucket_t *sl_bucket_prev(SL_NAME *sl, sl_bucket_t *bucket)
{
    sl_bucket_t *prev_bucket;
    
    
    sl_bucket_t *current = sl->buckets;
    while(current->next != bucket)
    {
        current = current->next;
    }
    prev_bucket = current;
    
    
    return prev_bucket;
}

bool sl_bucket_is_elm_within(const sl_bucket_t *bucket, const SL_TYPE *elm)
{
    uintptr_t
    ibegin = (uintptr_t) (bucket->elms),
    iend   = (uintptr_t) (bucket->elms + SL_BUCKET_SIZE),
    ielm   = (uintptr_t) (elm);
    return ielm >= ibegin && ielm < iend;
}

sl_iter_t sl_begin(SL_NAME *sl)
{
    sl_iter_t ret;
    ret.sl = sl;
    ret.next_ptr_entry = &sl->buckets->next_ptrs[sl->buckets->first_elm_idx];
    ret.elm_entry = &sl->buckets->elms[sl->buckets->first_elm_idx];
    return ret;
}

sl_iter_t sl_end(SL_NAME *sl)
{
    sl_iter_t ret;
    ret.sl = sl;
    ret.next_ptr_entry = &sl->tail->next_ptrs[SL_BUCKET_SIZE],
    ret.elm_entry = &sl->tail->elms[SL_BUCKET_SIZE];
    
    return ret;
}

sl_iter_t sl_iter_next(sl_iter_t it)
{
    it.next_ptr_entry += 1;
    it.elm_entry    += 1;
    it.elm_entry    = it.next_ptr_entry->next_entry;
    it.next_ptr_entry = it.next_ptr_entry->next;
    return it;
}

SL_TYPE *sl_iter_elm(sl_iter_t it)
{
    return &it.elm_entry->value;
}

sl_iter_t sl_iter_pop(sl_iter_t it)
{
    return sl_pop(it.sl, &it.elm_entry->value);
}

bool sl_iter_eq(sl_iter_t a, sl_iter_t b)
{
    return (a.next_ptr_entry == b.next_ptr_entry);
}

#endif

#undef sl_index_t
#undef SL_INDEX_MAX

#undef SL_BUCKET_SIZE
#undef SL_TYPE
#undef SL_NAME

#undef sl_bucket_t
#undef sl_entry_t
#undef sl_next_ptr_entry_t
#undef sl_init
#undef sl_put
#undef sl_pop
#undef sl_foreach
#undef sl_deinit
#undef sl_validate

#undef sl_bucket_init
#undef sl_bucket_put
#undef sl_bucket_pop
#undef sl_bucket_is_elm_within
#undef sl_bucket_first_elm
#undef sl_bucket_last_elm
#undef sl_bucket_prev

#undef SL_CAT_
#undef SL_CAT
