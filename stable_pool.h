#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdalign.h>
#include <string.h>

#if !defined(SP_TYPE) || !defined(SP_NAME)

    #error "SP_TYPE and SP_NAME must be defined"

#endif

#if !defined(SP_BUCKET_SIZE)

    #define SP_BUCKET_SIZE (511)

#endif

#if !defined(SP_ALLOC)

    #define SP_ALLOC sp_alloc_mem
    #define SP_FREE  sp_free_mem

#else

    #if !defined(SP_FREE)
        #error "If you define SP_ALLOC you must define SP_FREE"
    #elif !defined(SP_IMPL)
        #warning "Only define SP_ALLOC and SP_FREE when you also define SP_IMPL"
    #endif

#endif

#if !defined(SP_ALLOC_CTX)

    #define SP_ALLOC_CTX NULL

#endif

#define sp_index_t   int64_t
#define SP_INDEX_MAX INT64_MAX

#define SP_CAT_(a,b) a##b
#define SP_CAT(a,b) SP_CAT_(a,b)

#define sp_iter_t               SP_CAT(SP_NAME, _iter_t)
#define sp_bucket_t             SP_CAT(SP_NAME, _bucket_t)
#define sp_entry_t              SP_CAT(SP_NAME, _entry_t)
#define sp_next_ptr_entry_t     SP_CAT(SP_NAME, _next_ptr_entry_t)
#define sp_init                 SP_CAT(SP_NAME, _init)
#define sp_put                  SP_CAT(SP_NAME, _put)
#define sp_put_all              SP_CAT(SP_NAME, _put_all)
#define sp_pop                  SP_CAT(SP_NAME, _pop)
#define sp_foreach              SP_CAT(SP_NAME, _foreach)
#define sp_begin                SP_CAT(SP_NAME, _begin)
#define sp_end                  SP_CAT(SP_NAME, _end)
#define sp_deinit               SP_CAT(SP_NAME, _deinit)

#define sp_bucket_init          SP_CAT(SP_NAME, _bucket_init)
#define sp_bucket_put           SP_CAT(SP_NAME, _bucket_put)
#define sp_bucket_pop           SP_CAT(SP_NAME, _bucket_pop)
#define sp_bucket_is_elm_within SP_CAT(SP_NAME, _bucket_is_elm_within)
#define sp_get_containing_bucket SP_CAT(SP_NAME, _get_containing_bucket)
#define sp_bucket_first_elm     SP_CAT(SP_NAME, _bucket_first_elm)
#define sp_bucket_last_elm      SP_CAT(SP_NAME, _bucket_last_elm)
#define sp_bucket_prev          SP_CAT(SP_NAME, _bucket_prev)

#define sp_iter_next            SP_CAT(SP_NAME, _iter_next)
#define sp_iter_elm             SP_CAT(SP_NAME, _iter_elm)
#define sp_iter_pop             SP_CAT(SP_NAME, _iter_pop)
#define sp_iter_eq              SP_CAT(SP_NAME, _iter_eq)
#define sp_iter_is_end          SP_CAT(SP_NAME, _iter_is_end)

#define sp_alloc_mem            SP_CAT(SP_NAME, _alloc_mem)
#define sp_free_mem             SP_CAT(SP_NAME, _free_mem)

#define sp_validate SP_CAT(SP_NAME, _validate)

#define SP_ARR_LEN(arr) \
sizeof(arr) / sizeof(arr[0])

#define SP_FOREACH(sp, body)                                                                                                  \
do                                                                                                                            \
{                                                                                                                             \
    typeof(sp) sp_sp = sp;                                                                                                    \
    typeof(sp_sp->buckets) sp_bucket = sp_sp->buckets;                                                                        \
    if(sp_bucket)                                                                                                             \
    {                                                                                                                         \
        typeof(&sp_sp->buckets->elms[0]) sp_entryp = &sp_bucket->elms[sp_bucket->first_elm_idx];                              \
        typeof(&sp_sp->buckets->next_ptrs[0]) sp_next_ptrp = &sp_bucket->next_ptrs[sp_bucket->first_elm_idx];                 \
        typeof(&sp_sp->buckets->next_ptrs[0]) sp_lastentry = &sp_sp->tail->next_ptrs[ SP_ARR_LEN(sp_sp->buckets->elms) - 1 ]; \
        for(                                                                                                                  \
            ;                                                                                                                 \
            sp_next_ptrp != sp_lastentry ;                                                                                    \
            ++sp_next_ptrp,                                                                                                   \
            ++sp_entryp, sp_entryp = sp_next_ptrp->next_entry,                                                                \
            sp_next_ptrp = sp_next_ptrp->next                                                                                 \
        )                                                                                                                     \
        {                                                                                                                     \
            body                                                                                                              \
        }                                                                                                                     \
    }                                                                                                                         \
} while(0)

#define SP_IT \
((typeof(sp_entryp->value)*const) &sp_entryp->value)

typedef struct sp_entry_t
{
    typeof(SP_TYPE) value;
} sp_entry_t;

typedef struct sp_next_ptr_entry_t
{
    sp_entry_t *next_entry;
    struct sp_next_ptr_entry_t *next;
} sp_next_ptr_entry_t;

typedef struct sp_bucket_t
{
    sp_index_t first_elm_idx;
    sp_entry_t elms[SP_BUCKET_SIZE + 1];
    sp_next_ptr_entry_t next_ptrs[SP_BUCKET_SIZE + 1];
    struct sp_bucket_t *next;
} sp_bucket_t;

typedef struct SP_NAME
{
    sp_bucket_t *buckets;
    sp_bucket_t *tail;
    //sp_entry_t *last_elm;
    size_t count;
    size_t bucket_count;
} SP_NAME;

typedef struct sp_iter_t
{
    SP_NAME *sp;
    sp_entry_t *elm_entry;
    sp_next_ptr_entry_t *next_ptr_entry;
} sp_iter_t;

void sp_init(SP_NAME *sp);
SP_TYPE *sp_put(SP_NAME *sp, SP_TYPE new_elm);
void sp_put_all(SP_NAME *sp, SP_TYPE *elms, size_t nelms);
sp_iter_t sp_pop(SP_NAME *sp, SP_TYPE *elm);
void sp_foreach(const SP_NAME *sp, void(*f)(SP_TYPE*,void*), void *arg);
void sp_deinit(SP_NAME *sp);

sp_iter_t sp_begin(SP_NAME *sp);
sp_iter_t sp_end(SP_NAME *sp);
sp_iter_t sp_iter_next(sp_iter_t it);
SP_TYPE *sp_iter_elm(sp_iter_t it);
sp_iter_t sp_iter_pop(sp_iter_t it);
bool sp_iter_eq(sp_iter_t a, sp_iter_t b);
bool sp_iter_is_end(sp_iter_t it);

#define SP_IMPL
#if defined(SP_IMPL)

void sp_bucket_init(sp_bucket_t *bucket);
SP_TYPE *sp_bucket_put(SP_NAME *sp, sp_bucket_t *bucket, SP_TYPE new_elm);
bool sp_bucket_pop(SP_NAME *sp, sp_bucket_t *bucket, SP_TYPE *elm);
bool sp_bucket_is_elm_within(const sp_bucket_t *bucket, const SP_TYPE *elm);
sp_bucket_t *sp_get_containing_bucket(SP_NAME *sp, const SP_TYPE *elm);
sp_index_t sp_bucket_first_elm(sp_bucket_t *bucket);
sp_index_t sp_bucket_last_elm(sp_bucket_t *bucket);
sp_bucket_t *sp_bucket_prev(SP_NAME *sp, sp_bucket_t *bucket);

bool sp_validate(SP_NAME *sp);

void *sp_alloc_mem(void *ctx, size_t size, size_t alignment);
void sp_free_mem(void *ctx, void *ptr, size_t size);

void sp_init(SP_NAME *sp)
{
    (*sp) = (SP_NAME){
        .buckets      = NULL,
        .tail         = NULL,
        .count        = 0,
        .bucket_count = 0
    };
}

SP_TYPE *sp_put(SP_NAME *sp, SP_TYPE new_elm)
{
    SP_TYPE *elm_added = NULL;
    
    for(sp_bucket_t *bucket = sp->buckets ; bucket != NULL ; bucket = bucket->next)
    {
        elm_added = sp_bucket_put(sp, bucket, new_elm);
        if(elm_added != NULL)
        {
            sp->count += 1;
            sp_index_t last_elm_in_bucket = sp_bucket_last_elm(bucket);
            
            return elm_added;
        }
    }
    
    sp_bucket_t *new_bucket = (sp_bucket_t*) SP_ALLOC(SP_ALLOC_CTX, sizeof(sp_bucket_t), alignof(sp_bucket_t));
    sp_bucket_init(new_bucket);
    
    if(sp->count > 0)
        sp->tail->next = new_bucket;
    else
        sp->buckets = new_bucket;
    
    sp->bucket_count += 1;
    sp->tail = new_bucket;
    elm_added = sp_bucket_put(sp, new_bucket, new_elm);
    sp->count += 1;
    return elm_added;
}

void sp_put_all(SP_NAME *sp, SP_TYPE *elms, size_t nelms)
{
    size_t buckets_to_fill = nelms / SP_BUCKET_SIZE;
    size_t remaining = nelms - (buckets_to_fill * SP_BUCKET_SIZE);
    
    sp_bucket_t *first = NULL;
    bool first_set = false;
    sp_bucket_t *prev = NULL;
    sp_bucket_t *bucket = NULL;
    for(size_t i = 0 ; i < buckets_to_fill ; i++)
    {
        bucket = (sp_bucket_t*) SP_ALLOC(SP_ALLOC_CTX, sizeof(sp_bucket_t), alignof(sp_bucket_t));
        if(!first_set)
        {
            first = bucket;
            first_set = true;
        }
        sp_bucket_init(bucket);
        memcpy(bucket->elms, elms + (i * SP_BUCKET_SIZE), SP_BUCKET_SIZE * sizeof(SP_TYPE));
        for(sp_index_t j = 0 ; j < SP_BUCKET_SIZE ; j++)
        {
            bucket->next_ptrs[j].next = &bucket->next_ptrs[j];
            bucket->next_ptrs[j].next_entry = &bucket->elms[j];
        }
        bucket->first_elm_idx = 0;
        if(prev)
        {
            prev->next = bucket;
            prev->next_ptrs[SP_BUCKET_SIZE].next = &bucket->next_ptrs[0];
            prev->next_ptrs[SP_BUCKET_SIZE].next_entry = &bucket->elms[0];
        }
        prev = bucket;
        sp->bucket_count += 1;
        sp->count += SP_BUCKET_SIZE;
    }
    
    if(remaining > 0)
    {
        sp_bucket_t *remaining_bucket = (sp_bucket_t*) SP_ALLOC(SP_ALLOC_CTX, sizeof(sp_bucket_t), alignof(sp_bucket_t));;
        sp_bucket_init(remaining_bucket);
        memcpy(remaining_bucket->elms, elms + (buckets_to_fill * SP_BUCKET_SIZE), remaining * sizeof(SP_TYPE));
        for(sp_index_t j = 0 ; j < remaining ; j++)
        {
            remaining_bucket->next_ptrs[j].next = &remaining_bucket->next_ptrs[j];
            remaining_bucket->next_ptrs[j].next_entry = &remaining_bucket->elms[j];
        }
        remaining_bucket->first_elm_idx = 0;
        
        if(bucket)
        {
            bucket->next = remaining_bucket;
            bucket->next_ptrs[SP_BUCKET_SIZE].next = &remaining_bucket->next_ptrs[0];
            bucket->next_ptrs[SP_BUCKET_SIZE].next_entry = &remaining_bucket->elms[0];
        }
        
        bucket = remaining_bucket;
        
        if(!first)
        {
            first = remaining_bucket;
        }
        
        sp->count += remaining;
        sp->bucket_count += 1;
    }
    
    if(sp->tail != NULL && first != NULL)
    {
        sp_bucket_t *old_tail = sp->tail;
        sp->tail->next = first;
        sp->tail = sp->tail->next;
        sp_index_t last_in_tail = sp_bucket_last_elm(sp->tail);
        for(sp_index_t i = SP_BUCKET_SIZE ; i > last_in_tail ; i--)
        {
            old_tail->next_ptrs[i].next = &first->next_ptrs[0];
            old_tail->next_ptrs[i].next_entry = &first->elms[0];
        }
    }
    else
    {
        sp->tail = bucket;
        sp->buckets = first;
    }
}

sp_iter_t sp_pop(SP_NAME *sp, SP_TYPE *elm)
{
    sp_bucket_t *prev = NULL;
    sp_iter_t ret = {
        .sp = sp
    };
    for(sp_bucket_t *bucket = sp->buckets ; bucket != NULL ; bucket = bucket->next)
    {
        if(sp_bucket_is_elm_within(bucket, elm))
        {
            if(sp_bucket_pop(sp, bucket, elm))
            {
                if(prev != NULL)
                {
                    prev->next = bucket->next;
                }
                else
                {
                    sp->buckets = bucket->next;
                }
                
                if(bucket == sp->tail)
                {
                    sp->tail = prev;
                }
                
                if(bucket->next != NULL)
                {
                    sp_index_t first_index_of_next_bucket = sp_bucket_first_elm(bucket->next);
                    ret.next_ptr_entry = &bucket->next->next_ptrs[first_index_of_next_bucket];
                    ret.elm_entry = &bucket->next->elms[first_index_of_next_bucket];
                }
                else if(sp->tail != NULL)
                {
                    ret.next_ptr_entry = &sp->tail->next_ptrs[SP_BUCKET_SIZE];
                    ret.elm_entry = &sp->tail->elms[SP_BUCKET_SIZE];
                }
                SP_FREE(SP_ALLOC_CTX, bucket, sizeof(*bucket));
                sp->bucket_count -= 1;
            }
            else
            {
                sp_index_t next_elm = elm - (SP_TYPE*)bucket->elms + 1;
                for( ; next_elm < SP_BUCKET_SIZE ; next_elm++)
                {
                    ;
                }
                if(next_elm == SP_BUCKET_SIZE && bucket != sp->tail)
                {
                    sp_bucket_t *next_bucket = bucket->next;
                    sp_index_t first_elm_in_next_bucket = sp_bucket_first_elm(next_bucket);
                    ret.elm_entry = &next_bucket->elms[first_elm_in_next_bucket];
                    ret.next_ptr_entry = &next_bucket->next_ptrs[first_elm_in_next_bucket];
                }
                else
                {
                    ret.elm_entry = &bucket->elms[next_elm];
                    ret.next_ptr_entry = &bucket->next_ptrs[next_elm];
                }
            }
            sp->count -= 1;
            return ret;
        }
        prev = bucket;
    }
    return ret;
}

__attribute__((noinline))
void sp_foreach(const SP_NAME *sp, void(*f)(SP_TYPE*,void*), void *arg)
{
    if(sp->bucket_count == 0)
        return;
    sp_bucket_t *bucket = sp->buckets;
    sp_entry_t *entryp = &bucket->elms[bucket->first_elm_idx];
    sp_next_ptr_entry_t *next_ptrp = &bucket->next_ptrs[bucket->first_elm_idx];
    sp_next_ptr_entry_t *lastentry = &sp->tail->next_ptrs[SP_BUCKET_SIZE];
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

void sp_deinit(SP_NAME *sp)
{
    for(sp_bucket_t *current = sp->buckets ; current != NULL ; )
    {
        sp_bucket_t *next = current->next;
        SP_FREE(SP_ALLOC_CTX, current, sizeof(sp_bucket_t));
        current = next;
    }
    *sp = (SP_NAME){0};
}

void sp_bucket_init(sp_bucket_t *bucket)
{
    bucket->first_elm_idx = SP_INDEX_MAX;
    bucket->next = NULL;
    
    for(sp_index_t i = 0 ; i <= SP_BUCKET_SIZE ; i++)
    {
        bucket->next_ptrs[i].next_entry = &bucket->elms[SP_BUCKET_SIZE];
        bucket->next_ptrs[i].next = &bucket->next_ptrs[SP_BUCKET_SIZE];
    }
}

SP_TYPE *sp_bucket_put(SP_NAME *sp, sp_bucket_t *bucket, SP_TYPE new_elm)
{
    int emptyIndex = 0;
    for( ; emptyIndex < SP_BUCKET_SIZE ; emptyIndex++)
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
        // TODO make all the prev empty elms in this bucket next_ptr to the new first elm
        for(sp_index_t i = 0 ; i < bucket->first_elm_idx ; i++)
        {
            bucket->next_ptrs[i].next_entry = &bucket->elms[bucket->first_elm_idx];
            bucket->next_ptrs[i].next = &bucket->next_ptrs[bucket->first_elm_idx];
        }
        
        if(bucket != sp->buckets)
        {
            // we need to make the prev bucket link to this one's first elm
            sp_bucket_t *prev_bucket = sp_bucket_prev(sp, bucket);
            
            sp_index_t last_elm_in_prev_bucket = sp_bucket_last_elm(prev_bucket);
            for(sp_index_t k = last_elm_in_prev_bucket + 1 ; k < SP_BUCKET_SIZE + 1 ; k++)
            {
                prev_bucket->next_ptrs[k].next_entry = &bucket->elms[bucket->first_elm_idx];
                prev_bucket->next_ptrs[k].next = &bucket->next_ptrs[bucket->first_elm_idx];
            }
        }
    }
    
    return &bucket->elms[emptyIndex].value;
}

bool sp_bucket_pop(SP_NAME *sp, sp_bucket_t *bucket, SP_TYPE *elm)
{
    sp_entry_t *as_entry = (sp_entry_t*) elm;
    ptrdiff_t index = as_entry - bucket->elms;
    if(bucket->next_ptrs[index].next != &bucket->next_ptrs[index])
        return false;
    
    bool is_first_elm = false;
    sp_index_t index_of_first_elm = sp_bucket_first_elm(bucket);
    if(index == index_of_first_elm)
    {
        is_first_elm = true;
    }
    
    ptrdiff_t next_elm;
    for(next_elm = index + 1 ; next_elm < SP_BUCKET_SIZE && bucket->next_ptrs[next_elm].next != &bucket->next_ptrs[next_elm] ; next_elm++ )
    {
        ;
    }
    
    if(next_elm < SP_BUCKET_SIZE)
    {
        bucket->next_ptrs[index].next_entry = &bucket->elms[next_elm];
        bucket->next_ptrs[index].next = &bucket->next_ptrs[next_elm];
    }
    else
    {
        sp_bucket_t *next = bucket->next;
        if(next != NULL)
        {
            sp_index_t first_elm_in_next_bucket = sp_bucket_first_elm(next);
            bucket->next_ptrs[index].next = &next->next_ptrs[first_elm_in_next_bucket];
            bucket->next_ptrs[index].next_entry = &next->elms[first_elm_in_next_bucket];
        }
        else
        {
            bucket->next_ptrs[index].next = &bucket->next_ptrs[SP_BUCKET_SIZE];
            bucket->next_ptrs[index].next_entry = &bucket->elms[SP_BUCKET_SIZE];
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
        for(sp_index_t j = index + 1 ; j < SP_BUCKET_SIZE ; j++)
        {
            if(bucket->next_ptrs[j].next == &bucket->next_ptrs[j])
            {
                bucket->first_elm_idx = j;
                goto not_empty;
            }
        }
        is_empty = true;
        bucket->first_elm_idx = SP_INDEX_MAX; // this doesn't really matter, this node will get deleted since it's empty
    }
    
    not_empty:
    
    // if we just deleted the first elm
    // and current bucket is empty and is not the head
    // then go to the previous bucket and correct its next_ptrs
    if(is_first_elm && bucket != sp->buckets)
    {
        if(!is_empty)
        {
            // pick a new first_elm and make it link to the prev bucket
            
            sp_index_t first_elm_in_this_bucket = sp_bucket_first_elm(bucket);
            sp_bucket_t *prev_bucket = sp_bucket_prev(sp, bucket);
            
            sp_index_t last_elm_in_prev_bucket = sp_bucket_last_elm(prev_bucket);
            
            for(sp_index_t k = last_elm_in_prev_bucket + 1 ; k < SP_BUCKET_SIZE + 1 ; k++)
            {
                prev_bucket->next_ptrs[k].next_entry = &bucket->elms[first_elm_in_this_bucket];
                prev_bucket->next_ptrs[k].next = &bucket->next_ptrs[first_elm_in_this_bucket];
            }
        }
        else
        {
            // find the first elm in the next bucket, make the previous bucket next_ptr to it
            // if no next bucket, make the prev bucket point to its last dummy elm at [SP_BUCKET_SIZE]
            sp_bucket_t *next_bucket = bucket->next;
            
            if(next_bucket != NULL)
            {
                sp_bucket_t *prev_bucket = sp_bucket_prev(sp, bucket);
                sp_index_t first_elm_in_next_bucket = sp_bucket_first_elm(next_bucket);
                for(sp_index_t i = SP_BUCKET_SIZE ; prev_bucket->next_ptrs[i].next != &prev_bucket->next_ptrs[i] ; i--)
                {
                    prev_bucket->next_ptrs[i].next_entry = &next_bucket->elms[first_elm_in_next_bucket];
                    prev_bucket->next_ptrs[i].next = &next_bucket->next_ptrs[first_elm_in_next_bucket];
                }
            }
            else
            {
                // handle tail case
                // make the prev bucket link with its last index
                sp_bucket_t *prev_bucket = sp_bucket_prev(sp, bucket);
                sp_index_t last_elm_in_prev_bucket = sp_bucket_last_elm(prev_bucket);
                
                prev_bucket->next_ptrs[SP_BUCKET_SIZE].next_entry = &prev_bucket->elms[SP_BUCKET_SIZE]; //sizeof(sp_entry_t);
                prev_bucket->next_ptrs[SP_BUCKET_SIZE].next = &prev_bucket->next_ptrs[SP_BUCKET_SIZE];
                for(sp_index_t i = SP_BUCKET_SIZE - 1 ; i != last_elm_in_prev_bucket ; i--)
                {
                    prev_bucket->next_ptrs[i].next_entry = &prev_bucket->elms[SP_BUCKET_SIZE];
                    prev_bucket->next_ptrs[i].next = &prev_bucket->next_ptrs[SP_BUCKET_SIZE];
                }
            }
        }
    }
    
    return is_empty;
}

sp_index_t sp_bucket_first_elm(sp_bucket_t *bucket)
{
    return bucket->first_elm_idx;
    // for(sp_index_t i = 0 ; i < SP_BUCKET_SIZE ; i++)
    // {
    //     if(bucket->next_ptrs[i].elm_next_ptr == 0)
    //         return i;
    // }
    // return SP_INDEX_MAX;
}

sp_index_t sp_bucket_last_elm(sp_bucket_t *bucket)
{
    for(sp_index_t i = SP_BUCKET_SIZE - 1 ; i >= 0 ; i--)
    {
        if(bucket->next_ptrs[i].next == &bucket->next_ptrs[i])
            return i;
    }
    return SP_INDEX_MAX;
}

sp_bucket_t *sp_bucket_prev(SP_NAME *sp, sp_bucket_t *bucket)
{
    sp_bucket_t *prev_bucket;
    
    
    sp_bucket_t *current = sp->buckets;
    while(current->next != bucket)
    {
        current = current->next;
    }
    prev_bucket = current;
    
    
    return prev_bucket;
}

bool sp_bucket_is_elm_within(const sp_bucket_t *bucket, const SP_TYPE *elm)
{
    uintptr_t
    ibegin = (uintptr_t) (bucket->elms),
    iend   = (uintptr_t) (bucket->elms + SP_BUCKET_SIZE),
    ielm   = (uintptr_t) (elm);
    return ielm >= ibegin && ielm < iend;
}

sp_bucket_t *sp_get_containing_bucket(SP_NAME *sp, const SP_TYPE *elm)
{
    sp_bucket_t *bucket = sp->buckets;
    while(bucket != NULL)
    {
        if(sp_bucket_is_elm_within(bucket, elm))
        {
            return bucket;
        }
        bucket = bucket->next;
    }
    return NULL;
}

sp_iter_t sp_begin(SP_NAME *sp)
{
    sp_iter_t ret;
    ret.sp = sp;
    ret.next_ptr_entry = &sp->buckets->next_ptrs[sp->buckets->first_elm_idx];
    ret.elm_entry = &sp->buckets->elms[sp->buckets->first_elm_idx];
    return ret;
}

sp_iter_t sp_end(SP_NAME *sp)
{
    sp_iter_t ret;
    ret.sp = sp;
    ret.next_ptr_entry = &sp->tail->next_ptrs[SP_BUCKET_SIZE],
    ret.elm_entry = &sp->tail->elms[SP_BUCKET_SIZE];
    
    return ret;
}

sp_iter_t sp_iter_next(sp_iter_t it)
{
    it.next_ptr_entry += 1;
    it.elm_entry      += 1;
    it.elm_entry      = it.next_ptr_entry->next_entry;
    it.next_ptr_entry = it.next_ptr_entry->next;
    return it;
}

SP_TYPE *sp_iter_elm(sp_iter_t it)
{
    return &it.elm_entry->value;
}

sp_iter_t sp_iter_pop(sp_iter_t it)
{
    return sp_pop(it.sp, &it.elm_entry->value);
}

bool sp_iter_eq(sp_iter_t a, sp_iter_t b)
{
    return (a.next_ptr_entry == b.next_ptr_entry);
}

bool sp_iter_is_end(sp_iter_t it)
{
    return sp_iter_eq(it, sp_end(it.sp));
}

void *sp_alloc_mem(void *ctx, size_t size, size_t alignment)
{
    return malloc(size);
}

void sp_free_mem(void *ctx, void *ptr, size_t size)
{
    free(ptr);
}

bool sp_validate(SP_NAME *sp)
{
    sp_bucket_t *cb = sp->buckets;
    for(size_t i = 0 ; i < sp->bucket_count ; i++, cb = cb->next)
    {
        sp_index_t start_from = 0;
        sp_index_t next_elm_idx = sp_bucket_first_elm(cb);
        while(next_elm_idx != SP_INDEX_MAX)
        {
            for(sp_index_t j = start_from ; j < next_elm_idx ; j++)
            {
                if(cb->next_ptrs[j].next != &cb->next_ptrs[next_elm_idx])
                {
                    fprintf(stderr, "BADERR\n");
                }
            }
            
            if(cb == sp->tail)
            {
                start_from = next_elm_idx + 1;
                sp_index_t j;
                for(j = next_elm_idx + 1 ; j <= SP_BUCKET_SIZE && cb->next_ptrs[j].next != &cb->next_ptrs[j] ; j++)
                {
                    ;
                }
                
                if(j >= SP_BUCKET_SIZE + 1)
                {
                    break;
                }
                
                if(cb->next_ptrs[j].next != &cb->next_ptrs[j])
                {
                    fprintf(stderr, "BADERR SOMETHING DOESNT MAKE SENSE HAPPENED\n");
                    next_elm_idx = SP_INDEX_MAX;
                }
                else
                {
                    next_elm_idx = j;
                }
            }
            else
            {
                start_from = next_elm_idx + 1;
                sp_index_t j;
                for(j = next_elm_idx + 1 ; j < SP_BUCKET_SIZE && cb->next_ptrs[j].next != &cb->next_ptrs[j] ; j++)
                {
                    ;
                }
                if(j < SP_BUCKET_SIZE)
                    next_elm_idx = j;
                else
                    next_elm_idx = SP_INDEX_MAX;
                if(cb->next == NULL)
                {
                    fprintf(stderr, "BADERR SOMETHING DOESNT MAKE SENSE HAPPENED2\n");
                    fprintf(stderr, "IS_HEAD: %d\nIS_TAIL: %d\ni: %zu\n", cb == sp->buckets, cb == sp->tail, i);
                    next_elm_idx = SP_INDEX_MAX;
                }
            }
        }
    }
    return true;
}

#endif

#undef SP_TYPE
#undef SP_NAME
#undef SP_IMPL
#undef SP_ALLOC
#undef SP_FREE
#undef SP_ALLOC_CTX

#undef sp_index_t
#undef SP_INDEX_MAX

#undef sp_iter_t
#undef sp_bucket_t
#undef sp_entry_t
#undef sp_next_ptr_entry_t
#undef sp_init
#undef sp_put
#undef sp_pop
#undef sp_foreach
#undef sp_begin
#undef sp_end
#undef sp_deinit

#undef sp_bucket_init
#undef sp_bucket_put
#undef sp_bucket_pop
#undef sp_bucket_is_elm_within
#undef sp_bucket_first_elm
#undef sp_bucket_last_elm
#undef sp_bucket_prev

#undef sp_iter_next
#undef sp_iter_elm
#undef sp_iter_pop
#undef sp_iter_eq
#undef sp_iter_is_end

#undef sp_alloc_mem
#undef sp_free_mem

#undef SP_CAT_
#undef SP_CAT
