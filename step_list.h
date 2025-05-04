#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

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

#define sl_bucket_t       SL_CAT(SL_NAME, _bucket_t)
#define sl_entry_t        SL_CAT(SL_NAME, _entry_t)
#define sl_offset_entry_t SL_CAT(SL_NAME, _offset_entry_t)
#define sl_init           SL_CAT(SL_NAME, _init)
#define sl_put            SL_CAT(SL_NAME, _put)
#define sl_pop            SL_CAT(SL_NAME, _pop)
#define sl_loop           SL_CAT(SL_NAME, _loop)
#define sl_deinit         SL_CAT(SL_NAME, _deinit)
#define sl_validate       SL_CAT(SL_NAME, _validate)

#if !defined(SL_ONLY_DECL)
    #define sl_elms_offsets_offset  SL_CAT(SL_NAME, _elms_offsets_offset)
    #define sl_bucket_init          SL_CAT(SL_NAME, _bucket_init)
    #define sl_bucket_put           SL_CAT(SL_NAME, _bucket_put)
    #define sl_bucket_pop           SL_CAT(SL_NAME, _bucket_pop)
    #define sl_bucket_is_elm_within SL_CAT(SL_NAME, _bucket_is_elm_within)
    #define sl_bucket_first_elm     SL_CAT(SL_NAME, _bucket_first_elm)
    #define sl_bucket_last_elm      SL_CAT(SL_NAME, _bucket_last_elm)
    #define sl_bucket_prev          SL_CAT(SL_NAME, _bucket_prev)
#endif

#define ARR_LEN(arr) \
sizeof(arr) / sizeof(arr[0])

#define SL_FOREACH(sl, body)                                                                           \
do                                                                                                     \
{                                                                                                      \
    typeof(sl->buckets) bucket = sl->buckets;                                                          \
    typeof(&sl->buckets->elms[0]) entryp = &bucket->elms[bucket->first_elm_idx];                       \
    typeof(&sl->buckets->offsets[0]) offsetp = &bucket->offsets[bucket->first_elm_idx];                \
    typeof(&sl->buckets->offsets[0]) lastentry = &sl->tail->offsets[ ARR_LEN(sl->buckets->elms) ];        \
    for(                                                                                               \
        ;                                                                                              \
    offsetp != lastentry ;                                                                             \
    ++offsetp,                                                                                         \
    ++entryp, entryp = (typeof(&sl->buckets->elms[0]))((uintptr_t)entryp + offsetp->elm_offset),       \
        offsetp = (typeof(&sl->buckets->offsets[0]))((intptr_t)offsetp + offsetp->offset_entry_offset  \
        )                                                                                              \
    )                                                                                                  \
    {                                                                                                  \
        body                                                                                           \
    }                                                                                                  \
} while(0)

#define SL_IT \
((typeof(entryp->value)*const) &entryp->value)

typedef struct sl_entry_t
{
    typeof(SL_TYPE) value;
    // uintptr_t next_elm_offset;
} sl_entry_t;

typedef struct sl_offset_entry_t
{
    uintptr_t elm_offset;
    uintptr_t offset_entry_offset;
    // the issue is we can only go from one array to another if we know the index,
    // which we dont here
} sl_offset_entry_t;

typedef struct sl_bucket_t
{
    sl_index_t first_elm_idx;
    sl_entry_t elms[SL_BUCKET_SIZE + 1];
    sl_offset_entry_t offsets[SL_BUCKET_SIZE + 1];
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

void sl_init(SL_NAME *sl);
SL_TYPE *sl_put(SL_NAME *sl, SL_TYPE new_elm);
void sl_pop(SL_NAME *sl, SL_TYPE *elm);
void sl_loop(const SL_NAME *sl, void(*f)(SL_TYPE*,void*), void *arg);
void sl_deinit(SL_NAME *sl);

#if !defined(SL_ONLY_DECL)

// offset from offsets to elms
// used like
// elm_ptr = (uintptr_t)offsets[i] + sl_elms_offsets_offset;
const uintptr_t sl_elms_offsets_offset = offsetof(sl_bucket_t, elms) - offsetof(sl_bucket_t, offsets);

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

SL_TYPE *sl_bucket_put(SL_NAME *sl, sl_bucket_t *bucket, SL_TYPE new_elm)
{
    int emptyIndex = 0;
    for( ; emptyIndex < SL_BUCKET_SIZE ; emptyIndex++)
    {
        if(bucket->offsets[emptyIndex].elm_offset != 0)
            goto emptyIndexFound;
    }
    return NULL;
    
    emptyIndexFound:
    bucket->elms[emptyIndex].value = new_elm;
    
    bucket->offsets[emptyIndex].elm_offset = 0;
    bucket->offsets[emptyIndex].offset_entry_offset = 0;
    
    if(emptyIndex < bucket->first_elm_idx)
    {
        bucket->first_elm_idx = emptyIndex;
        uintptr_t addrs_of_first_elm_in_this_bucket = (uintptr_t) &bucket->elms[bucket->first_elm_idx];
        uintptr_t addrs_of_first_offset_in_this_bucket = (uintptr_t) &bucket->offsets[bucket->first_elm_idx];
        // TODO make all the prev empty elms in this bucket offset to the new first elm
        for(sl_index_t i = 0 ; i < bucket->first_elm_idx ; i++)
        {
            uintptr_t addrs_of_curr_elm = (uintptr_t) &bucket->elms[i];
            uintptr_t addrs_of_curr_offset = (uintptr_t) &bucket->offsets[i];
            bucket->offsets[i].elm_offset = addrs_of_first_elm_in_this_bucket - addrs_of_curr_elm;
            bucket->offsets[i].offset_entry_offset = addrs_of_first_offset_in_this_bucket - addrs_of_curr_offset;
        }
        
        if(bucket != sl->buckets)
        {
            // we need to make the prev bucket link to this one's first elm
            sl_bucket_t *prev_bucket = sl_bucket_prev(sl, bucket);
            
            sl_index_t last_elm_in_prev_bucket = sl_bucket_last_elm(prev_bucket);
            uintptr_t addrs_of_first_elm_in_this_bucket = (uintptr_t) &bucket->elms[bucket->first_elm_idx];
            uintptr_t addrs_of_first_offset_in_this_bucket = (uintptr_t) &bucket->offsets[bucket->first_elm_idx];
            for(sl_index_t k = last_elm_in_prev_bucket + 1 ; k < SL_BUCKET_SIZE + 1 ; k++)
            {
                uintptr_t addrs_of_curr_elm    = (uintptr_t) &prev_bucket->elms[k];
                uintptr_t addrs_of_curr_offset = (uintptr_t) &prev_bucket->offsets[k];
                prev_bucket->offsets[k].elm_offset = addrs_of_first_elm_in_this_bucket - addrs_of_curr_elm;
                prev_bucket->offsets[k].offset_entry_offset = addrs_of_first_offset_in_this_bucket - addrs_of_curr_offset;
            }
        }
    }
    
    return &bucket->elms[emptyIndex].value;
}

bool sl_bucket_is_elm_within(const sl_bucket_t *bucket, const SL_TYPE *elm)
{
    uintptr_t
    ibegin = (uintptr_t) (bucket->elms),
    iend   = (uintptr_t) (bucket->elms + SL_BUCKET_SIZE),
    ielm   = (uintptr_t) (elm);
    return ielm >= ibegin && ielm < iend;
}

bool sl_bucket_pop(SL_NAME *sl, sl_bucket_t *bucket, SL_TYPE *elm)
{
    sl_entry_t *as_entry = (sl_entry_t*) elm;
    ptrdiff_t index = as_entry - bucket->elms;
    if(bucket->offsets[index].elm_offset != 0)
        return false;
    
    bool is_first_elm = false;
    sl_index_t index_of_first_elm = sl_bucket_first_elm(bucket);
    if(index == index_of_first_elm)
    {
        is_first_elm = true;
    }
    
    // index + 1 is always safe to access because we have a dummy index (the actual size of the array is SL_BUCKET_SIZE + 1)
    bucket->offsets[index].elm_offset = bucket->offsets[index + 1].elm_offset + (1 * sizeof(sl_entry_t));
    bucket->offsets[index].offset_entry_offset = bucket->offsets[index + 1].offset_entry_offset + (1 * sizeof(sl_offset_entry_t));
    
    for(ptrdiff_t i = index - 1 ; i >= 0 && bucket->offsets[i].elm_offset != 0 ; i--)
    {
        bucket->offsets[i].elm_offset = bucket->offsets[index].elm_offset + ((index - i) * sizeof(sl_entry_t));
        bucket->offsets[i].offset_entry_offset = bucket->offsets[index].offset_entry_offset + ((index - i) * sizeof(sl_offset_entry_t));
    }
    
    bool is_empty = false;
    if(index == bucket->first_elm_idx)
    {
        for(sl_index_t j = index + 1 ; j < SL_BUCKET_SIZE ; j++)
        {
            if(bucket->offsets[j].elm_offset == 0)
            {
                bucket->first_elm_idx = j;
                goto not_empty;
            }
        }
        bool is_empty = true;
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
            for(last_elm_in_bucket = index - 1; bucket->offsets[last_elm_in_bucket].elm_offset != 0 ; last_elm_in_bucket--)
                ;
            
            sl->last_elm = &bucket->elms[last_elm_in_bucket];
        }
    }
    
    // if we just deleted the first elm
    // and current bucket is empty and is not the head
    // then go to the previous bucket and correct its offsets
    if(is_first_elm && bucket != sl->buckets)
    {
        if(!is_empty)
        {
            // pick a new first_elm and make it link to the prev bucket
            
            sl_index_t first_elm_in_this_bucket = sl_bucket_first_elm(bucket);
            sl_bucket_t *prev_bucket = sl_bucket_prev(sl, bucket);
            
            sl_index_t last_elm_in_prev_bucket = sl_bucket_last_elm(prev_bucket);
            uintptr_t addrs_of_first_elm_in_this_bucket = (uintptr_t) &bucket->elms[first_elm_in_this_bucket];
            uintptr_t addrs_of_first_offset_in_this_bucket = (uintptr_t) &bucket->offsets[first_elm_in_this_bucket];
            for(sl_index_t k = last_elm_in_prev_bucket + 1 ; k < SL_BUCKET_SIZE + 1 ; k++)
            {
                uintptr_t addrs_of_curr_elm = (uintptr_t) &prev_bucket->elms[k];
                uintptr_t addrs_of_curr_offset = (uintptr_t) &prev_bucket->offsets[k];
                prev_bucket->offsets[k].elm_offset = addrs_of_first_elm_in_this_bucket - addrs_of_curr_elm;
                prev_bucket->offsets[k].offset_entry_offset = addrs_of_first_offset_in_this_bucket - addrs_of_curr_offset;
            }
        }
        else
        {
            // find the first elm in the next bucket, make the previous bucket offset to it
            // if no next bucket, make the prev bucket point to its last dummy elm at [SL_BUCKET_SIZE]
            sl_bucket_t *next_bucket = bucket->next;
            
            if(next_bucket != NULL)
            {
                sl_bucket_t *prev_bucket = sl_bucket_prev(sl, bucket);
                sl_index_t first_elm_in_next_bucket = sl_bucket_first_elm(next_bucket);
                uintptr_t addrs_of_first_elm_in_next_bucket = (uintptr_t) &next_bucket->elms[first_elm_in_next_bucket];
                uintptr_t addrs_of_first_offset_in_next_bucket = (uintptr_t) &next_bucket->offsets[first_elm_in_next_bucket];
                for(sl_index_t i = SL_BUCKET_SIZE - 1 ; prev_bucket->offsets[i].elm_offset != 0 ; i--)
                {
                    uintptr_t addrs_of_curr_elm  = (uintptr_t) &prev_bucket->elms[i];
                    uintptr_t addrs_of_curr_offset = (uintptr_t) &prev_bucket->offsets[i];
                    prev_bucket->offsets[i].elm_offset = addrs_of_first_elm_in_next_bucket - addrs_of_curr_elm;
                    prev_bucket->offsets[i].offset_entry_offset = addrs_of_first_offset_in_next_bucket - addrs_of_curr_offset;
                }
            }
            else
            {
                // handle tail case
                // make the prev bucket link with its last index
                sl_bucket_t *prev_bucket = sl_bucket_prev(sl, bucket);
                sl_index_t last_elm_in_prev_bucket = sl_bucket_last_elm(prev_bucket);
                
                // we set the offset of the last elm to 1 for some reason
                // can't think of a better idea
                // maybe make it loop back to the first elm of the sl?
                prev_bucket->offsets[SL_BUCKET_SIZE].elm_offset = 0; //sizeof(sl_entry_t);
                prev_bucket->offsets[SL_BUCKET_SIZE].offset_entry_offset = 0;
                uintptr_t addrs_of_end_of_bucket_elm = (uintptr_t) &prev_bucket->elms[SL_BUCKET_SIZE];
                uintptr_t addrs_of_end_of_bucket_offset = (uintptr_t) &prev_bucket->offsets[SL_BUCKET_SIZE];
                for(sl_index_t i = SL_BUCKET_SIZE - 1 ; i != last_elm_in_prev_bucket ; i--)
                {
                    uintptr_t addrs_of_curr_elm = (uintptr_t) &prev_bucket->elms[i];
                    uintptr_t addrs_of_curr_offset = (uintptr_t) &prev_bucket->offsets[i];
                    prev_bucket->offsets[i].elm_offset = addrs_of_end_of_bucket_elm - addrs_of_curr_elm;
                    prev_bucket->offsets[i].elm_offset = addrs_of_end_of_bucket_offset - addrs_of_curr_offset;
                }
            }
        }
    }
    
    return is_empty;
}

sl_index_t sl_bucket_first_elm(sl_bucket_t *bucket)
{
    for(sl_index_t i = 0 ; i < SL_BUCKET_SIZE ; i++)
    {
        if(bucket->offsets[i].elm_offset == 0)
            return i;
    }
    return SL_INDEX_MAX;
}

sl_index_t sl_bucket_last_elm(sl_bucket_t *bucket)
{
    for(sl_index_t i = SL_BUCKET_SIZE - 1 ; i >= 0 ; i--)
    {
        if(bucket->offsets[i].elm_offset == 0)
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

void sl_bucket_init(sl_bucket_t *bucket)
{
    bucket->first_elm_idx = SL_INDEX_MAX;
    bucket->next = NULL;
    
    uintptr_t addrs_of_end_of_bucket_elm = (uintptr_t) &bucket->elms[SL_BUCKET_SIZE];
    uintptr_t addrs_of_end_of_bucket_offset = (uintptr_t) &bucket->offsets[SL_BUCKET_SIZE];
    for(sl_index_t i = 0 ; i < SL_BUCKET_SIZE ; i++)
    {
        uintptr_t addrs_of_curr_elm = (uintptr_t) &bucket->elms[i];
        uintptr_t addrs_of_curr_offset = (intptr_t) &bucket->offsets[i];
        bucket->offsets[i].elm_offset = addrs_of_end_of_bucket_elm - addrs_of_curr_elm;
        bucket->offsets[i].offset_entry_offset = addrs_of_end_of_bucket_offset - addrs_of_curr_offset;
    }
    bucket->offsets[SL_BUCKET_SIZE].elm_offset = 0; // sizeof(sl_entry_t);
    bucket->offsets[SL_BUCKET_SIZE].offset_entry_offset = 0;
}

SL_TYPE *sl_put(SL_NAME *sl, SL_TYPE new_elm)
{
    SL_TYPE *elm_added = NULL;
    
    for(sl_bucket_t *bucket = sl->buckets ; bucket != NULL ; bucket = bucket->next)
    {
        elm_added = sl_bucket_put(sl, bucket, new_elm);
        sl_entry_t *as_entry = (sl_entry_t*) elm_added;
        if(elm_added != NULL)
        {
            sl->count += 1;
            sl_index_t index = as_entry - bucket->elms;
            
            sl_index_t last_elm_in_bucket;
            for(last_elm_in_bucket = SL_BUCKET_SIZE - 1 ; last_elm_in_bucket >= 0 ; last_elm_in_bucket--)
            {
                if(bucket->offsets[last_elm_in_bucket].elm_offset == 0)
                    break;
            }
            
            if(bucket == sl->tail)
            {
                sl->last_elm = &bucket->elms[last_elm_in_bucket];
            }
            else
            {
                // TODO make the jump_list[i] go to the next bucket if it is elm_added
                if(as_entry - bucket->elms == last_elm_in_bucket)
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
    elm_added = sl_bucket_put(sl, new_bucket, new_elm);
    sl->count += 1;
    sl->last_elm = (sl_entry_t*) elm_added;
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
                {
                    sl->tail = prev;
                    sl->last_elm = (sl_entry_t*) sl_bucket_last_elm(sl->tail);
                }
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
    sl_bucket_t *bucket = sl->buckets;
    sl_entry_t *entryp = &bucket->elms[bucket->first_elm_idx];
    sl_offset_entry_t *offsetp = &bucket->offsets[bucket->first_elm_idx];
    sl_offset_entry_t *lastentry = &sl->tail->offsets[SL_BUCKET_SIZE];
    for(
        ;
        offsetp != lastentry ;
        ++offsetp,
        ++entryp, entryp = (sl_entry_t*)((uintptr_t)entryp + offsetp->elm_offset),
        offsetp = (sl_offset_entry_t*)((intptr_t)offsetp + offsetp->offset_entry_offset
        )
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

// bool sl_validate(SL_NAME *sl)
// {
//     sl_bucket_t *bucket = sl->buckets;
//     while(bucket != NULL)
//     {
//         sl_index_t first_elm_idx = sl_bucket_first_elm(bucket);
//         int count = 0;
//         for(sl_index_t i = first_elm_idx ; i < SL_BUCKET_SIZE + 1 ; ++i, i += bucket->elms[i].next_elm_offset)
//         {
//             if(bucket->offsets[i].elm_offset != 0)
//             {
//                 return false;
//             }
//             count++;
//         }
//         
//         int count2 = 0;
//         for(sl_index_t j = 0 ; j < SL_BUCKET_SIZE + 1 ; j++)
//         {
//             if(bucket->elms[j].next_elm_offset == 0)
//                 count2++;
//         }
//         
//         if(count != count2)
//         {
//             fprintf(stderr, "An element with next_elm_offset=0 was skipped over\n");
//         }
//         
//         sl_entry_t *current_entry = &bucket->elms[bucket->first_elm_idx];
//         sl_entry_t *next_entry_in_bucket = &bucket->elms[sl_bucket_first_elm(bucket)];
//         while(current_entry != &sl->tail->elms[SL_BUCKET_SIZE])
//         {
//             sl_entry_t *next_entry = current_entry + current_entry->next_elm_offset;
//             if(next_entry != next_entry_in_bucket)
//             {
//                 fprintf(stderr, "incorrect offset\n");
//             }
//             
//             current_entry++;
//             if(current_entry > next_entry_in_bucket)
//             {
//                 next_entry_in_bucket = next_entry_in_bucket + 1;
//                 next_entry_in_bucket += next_entry_in_bucket->next_elm_offset;
//             }
//         }
//         
//         bucket = bucket->next;
//     }
//     return true;
// }

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
