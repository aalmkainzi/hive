#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdalign.h>
#include <string.h>
#include <time.h>
#include <assert.h>
// OPTIMIZATION IDEA:
// use a priority queue in the bucket's empty slots instead of a stack.
// this makes it so that each time we insert a new elm, to the left of it is either another elm or its at 0
// with this in mind we can make the insertion and erasing logic much faster
#if !defined(SP_TYPE) || !defined(SP_NAME)

    #error "SP_TYPE and SP_NAME must be defined"

#endif

#if !defined(SP_BUCKET_SIZE)

    #define SP_BUCKET_SIZE (UINT16_MAX - 1)

#endif

#if !defined(SP_ALLOC)

    #define SP_ALLOC   sp_alloc_mem
    #define SP_FREE    sp_free_mem
    #define SP_REALLOC sp_realloc_mem

#else

    #if !defined(SP_FREE) || !defined(SP_REALLOC)
        #error "If you define SP_ALLOC you must also define SP_FREE and SP_REALLOC"
    #elif !defined(SP_IMPL)
        #warning "Only define allocator macros when you also define SP_IMPL"
    #endif

#endif

#if !defined(SP_ALLOC_CTX)

    #define SP_ALLOC_CTX NULL

#endif

#if SP_BUCKET_SIZE > (UINT16_MAX)

    #define sp_index_t   uint32_t
    #define SP_INDEX_MAX UINT32_MAX

#elif SP_BUCKET_SIZE > (UINT8_MAX)

    #define sp_index_t   uint16_t
    #define SP_INDEX_MAX UINT16_MAX

#else

    #define sp_index_t   uint8_t
    #define SP_INDEX_MAX UINT8_MAX

#endif

#if defined(__GNUC__)

    #define SP_LIKELY(x)   __builtin_expect(x, 1)
    #define SP_UNLIKELY(x) __builtin_expect(x, 0)

#else

    #define SP_LIKELY(x)   x
    #define SP_UNLIKELY(x) x

#endif

#define SP_CAT_(a,b) a##b
#define SP_CAT(a,b) SP_CAT_(a,b)

#define sp_iter_t                SP_CAT(SP_NAME, _iter_t)
#define sp_bucket_t              SP_CAT(SP_NAME, _bucket_t)
#define sp_entry_t               SP_CAT(SP_NAME, _entry_t)
#define sp_offset_entry_t        SP_CAT(SP_NAME, _offset_entry_t)
#define sp_init                  SP_CAT(SP_NAME, _init)
#define sp_clone                 SP_CAT(SP_NAME, _clone)
#define sp_push_not_full_bucket  SP_CAT(SP_NAME, _push_not_full_bucket)
#define sp_put                   SP_CAT(SP_NAME, _put)
#define sp_put_all               SP_CAT(SP_NAME, _put_all)
#define sp_pop_helper            SP_CAT(SP_NAME, _pop_helper)
#define sp_pop                   SP_CAT(SP_NAME, _pop)
#define sp_foreach               SP_CAT(SP_NAME, _foreach)
#define sp_foreach_updater       SP_CAT(SP_NAME, _foreach_updater)
#define sp_begin                 SP_CAT(SP_NAME, _begin)
#define sp_end                   SP_CAT(SP_NAME, _end)
#define sp_deinit                SP_CAT(SP_NAME, _deinit)

#define sp_bucket_init           SP_CAT(SP_NAME, _bucket_init)
#define sp_bucket_put            SP_CAT(SP_NAME, _bucket_put)
#define sp_bucket_pop            SP_CAT(SP_NAME, _bucket_pop)
#define sp_bucket_is_elm_within  SP_CAT(SP_NAME, _bucket_is_elm_within)
#define sp_get_containing_bucket SP_CAT(SP_NAME, _get_containing_bucket)
#define sp_bucket_first_elm      SP_CAT(SP_NAME, _bucket_first_elm)
#define sp_bucket_prev           SP_CAT(SP_NAME, _bucket_prev)

#define sp_iter_next             SP_CAT(SP_NAME, _iter_next)
#define sp_iter_go_next          SP_CAT(SP_NAME, _iter_go_next)
#define sp_iter_elm              SP_CAT(SP_NAME, _iter_elm)
#define sp_iter_pop              SP_CAT(SP_NAME, _iter_pop)
#define sp_iter_eq               SP_CAT(SP_NAME, _iter_eq)
#define sp_iter_is_end           SP_CAT(SP_NAME, _iter_is_end)
#define sp_iter_to               SP_CAT(SP_NAME, _iter_to)

#define sp_alloc_mem             SP_CAT(SP_NAME, _alloc_mem)
#define sp_realloc_mem           SP_CAT(SP_NAME, _realloc_mem)
#define sp_free_mem              SP_CAT(SP_NAME, _free_mem)

#define sp_validate SP_CAT(SP_NAME, _validate)

#define SP_ARR_LEN(arr) \
sizeof(arr) / sizeof(arr[0])

#define SP_GET_BUCKET_SIZE(sp) \
(SP_ARR_LEN((sp)->buckets->elms) - 1)

#define SP_FOREACH(sp, body)                                                                   \
do                                                                                             \
{                                                                                              \
    typeof(*sp) * const sp_sp = sp;                                                            \
    const typeof(*sp_sp->buckets) *const sp_end_bucket = sp_sp->end_sentinel;                  \
    typeof(sp_sp->buckets) sp_bucket = sp_sp->buckets;                                         \
    typeof(sp_bucket->offsets[0].next_elm_index) sp_index = sp_bucket->first_elm_idx;          \
    const typeof(sp_bucket->offsets[0]) *sp_offsets_base = &sp_bucket->offsets[0];             \
    typeof(*sp_bucket->elms) *sp_elms_base = &sp_bucket->elms[0];                              \
    while( sp_bucket != sp_end_bucket )                                                        \
    {                                                                                          \
        { body }                                                                               \
                                                                                               \
        (sp_index) = sp_offsets_base[sp_index + 1].next_elm_index;                             \
        if(SP_UNLIKELY(sp_index >= SP_GET_BUCKET_SIZE(sp_sp)))                                 \
        {                                                                                      \
            (sp_bucket) = (sp_bucket)->next;                                                   \
            (sp_index) = (sp_bucket)->first_elm_idx;                                           \
            sp_offsets_base = &sp_bucket->offsets[0];                                          \
            sp_elms_base = &sp_bucket->elms[0];                                                \
        }                                                                                      \
    }                                                                                          \
} while(0)

#define SP_IT \
((typeof(sp_bucket->elms[0].value)*const) &sp_elms_base[sp_index])

typedef struct sp_entry_t
{
    typeof(SP_TYPE) value;
} sp_entry_t;

typedef struct sp_offset_entry_t
{
    sp_index_t next_elm_index;
} sp_offset_entry_t;

typedef struct sp_bucket_t
{
    uint16_t not_full_idx; // if this bucket is not full, this will be set to its index inside the `not_full_buckets` array
    sp_index_t first_elm_idx;
    sp_index_t count;
    sp_entry_t elms[SP_BUCKET_SIZE + 1];
    sp_offset_entry_t offsets[SP_BUCKET_SIZE + 1];
    sp_index_t empty_indexes[SP_BUCKET_SIZE];
    struct sp_bucket_t *next;
    struct sp_bucket_t *prev;
} sp_bucket_t;

typedef struct SP_NAME
{
    sp_bucket_t *buckets;
    sp_bucket_t *tail;
    sp_bucket_t *end_sentinel; // tail->next == end_sentinel
    struct
    {
        sp_bucket_t **array;
        uint16_t count;
        uint16_t cap;
    } not_full_buckets;
    size_t count;
    size_t bucket_count;
} SP_NAME;

typedef struct sp_iter_t
{
    SP_NAME *sp;
    sp_bucket_t *bucket;
    sp_offset_entry_t *offset;
    sp_entry_t *elm;
} sp_iter_t;

void sp_init(SP_NAME *sp);
SP_NAME sp_clone(const SP_NAME *const sp);
SP_TYPE *sp_put(SP_NAME *sp, SP_TYPE new_elm);
void sp_put_all(SP_NAME *sp, SP_TYPE *elms, size_t nelms);
sp_iter_t sp_pop(SP_NAME *sp, SP_TYPE *elm);
void sp_foreach(const SP_NAME *sp, void(*f)(SP_TYPE*,void*), void *arg);
void sp_foreach_updater(sp_index_t *index, sp_bucket_t **bucket);
void sp_deinit(SP_NAME *sp);

sp_iter_t sp_begin(SP_NAME *sp);
sp_iter_t sp_end(SP_NAME *sp);
sp_iter_t sp_iter_next(sp_iter_t it);
void sp_iter_go_next(sp_iter_t *it);
SP_TYPE *sp_iter_elm(sp_iter_t it);
sp_iter_t sp_iter_pop(sp_iter_t it);
bool sp_iter_eq(sp_iter_t a, sp_iter_t b);
bool sp_iter_is_end(sp_iter_t it);

#define SP_IMPL
#if defined(SP_IMPL)

void sp_bucket_init(sp_bucket_t *bucket);
void sp_push_not_full_bucket(SP_NAME *sp, sp_bucket_t *bucket);
SP_TYPE *sp_bucket_put(SP_NAME *sp, sp_bucket_t *bucket, SP_TYPE new_elm);
bool sp_bucket_pop(SP_NAME *sp, sp_bucket_t *bucket, sp_index_t index);
sp_iter_t sp_pop_helper(SP_NAME *sp, sp_bucket_t *prev_bucket, sp_bucket_t *bucket, sp_index_t index);
bool sp_bucket_is_elm_within(const sp_bucket_t *bucket, const SP_TYPE *elm);
sp_bucket_t *sp_get_containing_bucket(SP_NAME *sp, const SP_TYPE *elm);
sp_index_t sp_bucket_first_elm(sp_bucket_t *bucket);
sp_bucket_t *sp_bucket_prev(SP_NAME *sp, sp_bucket_t *bucket);
sp_iter_t sp_iter_to(SP_NAME *sp, sp_bucket_t *bucket, sp_index_t index);

bool sp_validate(SP_NAME *sp);

void *sp_alloc_mem(void *ctx, size_t size, size_t alignment);
void *sp_realloc_mem(void *ctx, void *ptr, size_t old_size, size_t new_size, size_t alignment);
void sp_free_mem(void *ctx, void *ptr, size_t size);

#define SP_ALLOC_N(type, count) \
(typeof(type)*) SP_ALLOC(SP_ALLOC_CTX, sizeof(type) * count, alignof(type))

#define SP_REALLOC_N(ptr, old_count, new_count) \
(typeof(ptr)) SP_REALLOC(SP_ALLOC_CTX, (void*)(ptr), sizeof((ptr)[0]) * (old_count), sizeof((ptr)[0]) * (new_count), alignof(typeof((ptr)[0])))

#define SP_FREE_N(ptr, count) \
SP_FREE(SP_ALLOC_CTX, (void*)ptr, sizeof(ptr[0]) * count)

void sp_init(SP_NAME *sp)
{
    sp_bucket_t *end_sentinel = (sp_bucket_t*) SP_ALLOC(SP_ALLOC_CTX, sizeof(sp_bucket_t), alignof(sp_bucket_t));
    sp_bucket_init(end_sentinel);
    end_sentinel->first_elm_idx = SP_BUCKET_SIZE;
    (*sp) = (SP_NAME){
        .buckets      = end_sentinel,
        .tail         = end_sentinel,
        .end_sentinel = end_sentinel,
        .count        = 0,
        .bucket_count = 0,
    };
    sp->not_full_buckets.count = 0;
    sp->not_full_buckets.cap = 16;
    sp->not_full_buckets.array = SP_ALLOC_N(sp_bucket_t*, sp->not_full_buckets.cap);
}

SP_NAME sp_clone(const SP_NAME *const sp)
{
    SP_NAME ret;
    sp_init(&ret);
    ret.count = sp->count;
    ret.bucket_count = sp->bucket_count;
    
    if(sp->not_full_buckets.count > ret.not_full_buckets.cap)
    {
        ret.not_full_buckets.array = SP_REALLOC_N(ret.not_full_buckets.array, ret.not_full_buckets.cap, sp->not_full_buckets.count + 1);
        ret.not_full_buckets.cap = sp->not_full_buckets.count + 1;
    }
    
    ret.not_full_buckets.count = sp->not_full_buckets.count;
    
    if(sp->bucket_count > 0)
    {
        sp_bucket_t *src_bucket = sp->buckets;
        
        sp_bucket_t **dst_bucket = &ret.buckets;
        sp_bucket_t *dst_prev = NULL;
        
        *dst_bucket = (sp_bucket_t*) SP_ALLOC(SP_ALLOC_CTX, sizeof(sp_bucket_t), alignof(sp_bucket_t));
        memcpy(*dst_bucket, src_bucket, sizeof(sp_bucket_t));
        (*dst_bucket)->prev = NULL;
        
        if((*dst_bucket)->not_full_idx != UINT16_MAX)
        {
            ret.not_full_buckets.array[(*dst_bucket)->not_full_idx] = *dst_bucket;
        }
        
        (*dst_bucket)->next = NULL;
        dst_prev = (*dst_bucket);
        dst_bucket = &(*dst_bucket)->next;
        src_bucket = src_bucket->next;
        
        while(src_bucket != sp->end_sentinel)
        {
            *dst_bucket = (sp_bucket_t*) SP_ALLOC(SP_ALLOC_CTX, sizeof(sp_bucket_t), alignof(sp_bucket_t));
            memcpy(*dst_bucket, src_bucket, sizeof(sp_bucket_t));
            (*dst_bucket)->next = NULL;
            (*dst_bucket)->prev = dst_prev;
            if((*dst_bucket)->not_full_idx != UINT16_MAX)
            {
                ret.not_full_buckets.array[(*dst_bucket)->not_full_idx] = *dst_bucket;
            }
            
            dst_prev->next = *dst_bucket;
            dst_prev = (*dst_bucket);
            dst_bucket = &(*dst_bucket)->next;
            src_bucket = src_bucket->next;
        }
        ret.tail = dst_prev;
        ret.tail->next = ret.end_sentinel;
    }
    else
    {
        ret.buckets = ret.tail = ret.end_sentinel;
    }
    
    return ret;
}

SP_TYPE *sp_put(SP_NAME *sp, SP_TYPE new_elm)
{
    SP_TYPE *elm_added = NULL;
    
    if(sp->not_full_buckets.count > 0)
    {
        sp_bucket_t *bucket = sp->not_full_buckets.array[sp->not_full_buckets.count - 1];
        sp->count += 1;
        return sp_bucket_put(sp, bucket, new_elm);
    }
    
    sp_bucket_t *new_bucket = (sp_bucket_t*) SP_ALLOC(SP_ALLOC_CTX, sizeof(sp_bucket_t), alignof(sp_bucket_t));
    sp_bucket_init(new_bucket);
    sp_push_not_full_bucket(sp, new_bucket);
    
    if(sp->count > 0)
    {
        sp->tail->next = new_bucket;
        new_bucket->next = sp->end_sentinel;
        sp->tail = new_bucket;
    }
    else
    {
         sp->buckets = new_bucket;
         sp->tail = new_bucket;
         new_bucket->next = sp->end_sentinel;
    }
    
    sp->bucket_count += 1;
    elm_added = sp_bucket_put(sp, new_bucket, new_elm);
    sp->count += 1;
    return elm_added;
}

void sp_put_all(SP_NAME *sp, SP_TYPE *elms, size_t nelms)
{
    size_t buckets_to_fill = nelms / SP_BUCKET_SIZE;
    ptrdiff_t remaining = nelms - (buckets_to_fill * SP_BUCKET_SIZE);
    
    sp_bucket_t *first = NULL;
    bool first_set = false;
    sp_bucket_t *bucket = NULL;
    sp_bucket_t *prev = NULL;
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
        for(sp_index_t j = 0 ; j <= SP_BUCKET_SIZE ; j++)
        {
            bucket->offsets[j].next_elm_index = j;
        }
        bucket->first_elm_idx = 0;
        bucket->count = SP_BUCKET_SIZE;
        sp->bucket_count += 1;
        sp->count += SP_BUCKET_SIZE;
        
        bucket->prev = prev;
        if(prev)
        {
            prev->next = bucket;
        }
        prev = bucket;
    }
    
    if(remaining > 0)
    {
        sp_bucket_t *remaining_bucket = (sp_bucket_t*) SP_ALLOC(SP_ALLOC_CTX, sizeof(sp_bucket_t), alignof(sp_bucket_t));;
        sp_bucket_init(remaining_bucket);
        memcpy(remaining_bucket->elms, elms + (buckets_to_fill * SP_BUCKET_SIZE), remaining * sizeof(SP_TYPE));
        for(sp_index_t j = 0 ; j < remaining ; j++)
        {
            remaining_bucket->offsets[j].next_elm_index = j;
        }
        remaining_bucket->first_elm_idx = 0;
        remaining_bucket->count = remaining;
        
        sp_push_not_full_bucket(sp, remaining_bucket);
        
        if(bucket)
        {
            bucket->next = remaining_bucket;
        }
        
        remaining_bucket->prev = bucket;
        bucket = remaining_bucket;
        
        if(!first)
        {
            first = remaining_bucket;
        }
        
        sp->count += remaining;
        sp->bucket_count += 1;
    }
    
    if(sp->buckets != sp->end_sentinel && first != NULL)
    {
        sp->tail->next = first;
        sp->tail = bucket;
        sp->tail->next = sp->end_sentinel;
        sp->end_sentinel->prev = sp->tail;
    }
    else
    {
        sp->buckets = first;
        sp->tail = bucket;
        sp->tail->next = sp->end_sentinel;
        sp->end_sentinel->prev = sp->tail;
    }
}

sp_iter_t sp_pop_helper(SP_NAME *sp, sp_bucket_t *prev_bucket, sp_bucket_t *bucket, sp_index_t index)
{
    sp_iter_t ret = {
        .sp = sp
    };
    if(sp_bucket_pop(sp, bucket, index))
    {
        if(prev_bucket != NULL)
        {
            prev_bucket->next = bucket->next;
        }
        else
        {
            sp->buckets = bucket->next;
        }
        bucket->next->prev = prev_bucket;
        
        if(bucket == sp->tail)
        {
            if(prev_bucket != NULL)
            {
                sp->tail = prev_bucket;
            }
            else
            {
                sp->tail = sp->end_sentinel;
                sp->end_sentinel->prev = NULL;
            }
            
            ret = sp_iter_to(sp, sp->end_sentinel, SP_BUCKET_SIZE);
        }
        else
        {
            sp_index_t first_elm = sp_bucket_first_elm(bucket->next);
            ret = sp_iter_to(sp, bucket->next, first_elm);
        }
        
        SP_FREE(SP_ALLOC_CTX, bucket, sizeof(*bucket));
        sp->bucket_count -= 1;
    }
    else
    {
        sp_index_t next_elm = bucket->offsets[index + 1].next_elm_index;
        
        if(next_elm == SP_BUCKET_SIZE)
        {
            sp_bucket_t *next_bucket = bucket->next;
            sp_index_t first_elm_in_next_bucket = sp_bucket_first_elm(next_bucket);
            ret = sp_iter_to(sp, next_bucket, first_elm_in_next_bucket);
        }
        else
        {
            ret = sp_iter_to(sp, bucket, next_elm);
        }
    }
    sp->count -= 1;
    return ret;
}


sp_iter_t sp_pop(SP_NAME *sp, SP_TYPE *elm)
{
    sp_bucket_t *prev = NULL;
    sp_iter_t ret = {
        .sp = sp
    };
    for(sp_bucket_t *bucket = sp->buckets ; bucket != sp->end_sentinel ; prev = bucket, bucket = bucket->next)
    {
        if(sp_bucket_is_elm_within(bucket, elm))
        {
            sp_entry_t *as_entry = (sp_entry_t*) elm;
            sp_index_t index = as_entry - bucket->elms;
            ret = sp_pop_helper(sp, prev, bucket, index);
            break;
        }
    }
    return ret;
}

void sp_foreach_updater(sp_index_t *index, sp_bucket_t **bucket)
{
    ++(*index);
    (*index) = (*bucket)->offsets[*index].next_elm_index;
    if(*index == SP_BUCKET_SIZE)
    {
        (*bucket) = (*bucket)->next;
        (*index) = (*bucket)->first_elm_idx;
    }
}

void sp_foreach(const SP_NAME *sp, void(*f)(SP_TYPE*,void*), void *arg)
{
    sp_bucket_t *bucket = sp->buckets;
    sp_index_t index = bucket->first_elm_idx;
    for( ; bucket != sp->end_sentinel ; sp_foreach_updater(&index, &bucket))
    {
        f(&bucket->elms[index].value, arg);
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
    SP_FREE_N(sp->not_full_buckets.array, sp->not_full_buckets.count);
    *sp = (SP_NAME){0};
}

void sp_bucket_init(sp_bucket_t *bucket)
{
    bucket->first_elm_idx = SP_BUCKET_SIZE;
    bucket->next = NULL;
    bucket->prev = NULL;
    bucket->not_full_idx = UINT16_MAX;
    bucket->count = 0;
    
    sp_index_t i;
    for(i = 0 ; i < SP_BUCKET_SIZE ; i++)
    {
        bucket->offsets[i].next_elm_index = SP_BUCKET_SIZE;
    }
    bucket->offsets[SP_BUCKET_SIZE].next_elm_index = SP_BUCKET_SIZE;
    
    for(sp_index_t j = 0 ; j < SP_BUCKET_SIZE ; j++)
    {
        bucket->empty_indexes[j] = SP_BUCKET_SIZE - j - 1;
    }
}

void sp_push_not_full_bucket(SP_NAME *sp, sp_bucket_t *bucket)
{
    if(sp->not_full_buckets.cap <= sp->not_full_buckets.count)
    {
        uint16_t new_cap = sp->not_full_buckets.cap * 2;
        sp->not_full_buckets.array = SP_REALLOC_N(sp->not_full_buckets.array, sp->not_full_buckets.cap, new_cap);
        sp->not_full_buckets.cap = new_cap;
    }
    
    sp->not_full_buckets.array[sp->not_full_buckets.count] = bucket;
    bucket->not_full_idx = sp->not_full_buckets.count;
    
    sp->not_full_buckets.count += 1;
}

SP_TYPE *sp_bucket_put(SP_NAME *sp, sp_bucket_t *bucket, SP_TYPE new_elm)
{
    // TODO no need to check anymore, we guaranteed before calling this function that this bucket is not full
    if(bucket->count >= SP_BUCKET_SIZE)
        return NULL;
    
    int emptyIndex = bucket->empty_indexes[SP_BUCKET_SIZE - bucket->count - 1];
    
    bucket->elms[emptyIndex].value = new_elm;
    bucket->offsets[emptyIndex].next_elm_index = emptyIndex;
    
    if(emptyIndex < bucket->first_elm_idx)
    {
        bucket->first_elm_idx = emptyIndex;
    }
    
    for(ptrdiff_t i = emptyIndex - 1 ; i >= 0 && bucket->offsets[i].next_elm_index != i ; i--)
    {
        bucket->offsets[i].next_elm_index = emptyIndex;
    }
    
    bucket->count += 1;
    if(bucket->count == SP_BUCKET_SIZE)
    {
        sp->not_full_buckets.count -= 1;
    }
    return &bucket->elms[emptyIndex].value;
}

bool sp_bucket_pop(SP_NAME *sp, sp_bucket_t *bucket, sp_index_t index)
{
    (void)sp;
    assert(bucket->offsets[index].next_elm_index == index);
    
    bucket->offsets[index].next_elm_index = bucket->offsets[index + 1].next_elm_index;
    
#if 0 && SP_INDEX_MAX == UINT8_MAX
    ptrdiff_t prev_elm;
    for(prev_elm = (ptrdiff_t)index - 1 ; prev_elm >= 0 && bucket->offsets[prev_elm].next_elm_index != prev_elm ; prev_elm--);
    prev_elm += 1;
    memset(bucket->offsets + prev_elm, bucket->offsets[index].next_elm_index, index - prev_elm);
    
#else
    for(ptrdiff_t i = (ptrdiff_t)index - 1 ; i >= 0 && bucket->offsets[i].next_elm_index != i ; i--)
    {
        bucket->offsets[i].next_elm_index = bucket->offsets[index].next_elm_index;
    }
#endif
    
    bool is_empty = false;
    if(SP_UNLIKELY(index == bucket->first_elm_idx))
    {
        sp_index_t next_elm = bucket->offsets[index].next_elm_index;
        bucket->first_elm_idx = next_elm;
        
        if(next_elm == SP_BUCKET_SIZE)
        {
            is_empty = true;
            sp->not_full_buckets.array[bucket->not_full_idx] = sp->not_full_buckets.array[sp->not_full_buckets.count-1];
            sp->not_full_buckets.array[bucket->not_full_idx]->not_full_idx = bucket->not_full_idx;
            sp->not_full_buckets.count -= 1;
            
            bucket->count -= 1;
            return is_empty;
        }
    }
    
    if(SP_UNLIKELY(bucket->count == SP_BUCKET_SIZE))
        sp_push_not_full_bucket(sp, bucket);
    bucket->empty_indexes[SP_BUCKET_SIZE - bucket->count] = index;
    bucket->count -= 1;
    
    return is_empty;
}

sp_index_t sp_bucket_first_elm(sp_bucket_t *bucket)
{
    return bucket->first_elm_idx;
}

sp_bucket_t *sp_bucket_prev(SP_NAME *sp, sp_bucket_t *bucket)
{
//     sp_bucket_t *current = sp->buckets;
//     while(current->next != bucket)
//     {
//         current = current->next;
//     }
//     
//     return current;
    
    return bucket->prev;
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
    return sp_iter_to(sp, sp->buckets, sp->buckets->first_elm_idx);
}

sp_iter_t sp_end(SP_NAME *sp)
{
    return sp_iter_to(sp, sp->end_sentinel, SP_BUCKET_SIZE);
}

sp_iter_t sp_iter_to(SP_NAME *sp, sp_bucket_t *bucket, sp_index_t index)
{
    sp_iter_t ret;
    ret.sp = sp;
    ret.bucket = bucket;
    ret.elm = &bucket->elms[index];
    ret.offset = &ret.bucket->offsets[index] + 1;
    return ret;
}

bool sp_iter_eq(sp_iter_t a, sp_iter_t b)
{
    return (a.elm == b.elm);
}

bool sp_iter_is_end(sp_iter_t it)
{
    return (it.bucket == it.sp->end_sentinel);
}

sp_iter_t sp_iter_next(sp_iter_t it)
{
    sp_index_t index = it.offset->next_elm_index;
    
    if (SP_UNLIKELY(index == SP_BUCKET_SIZE))
    {
        it.bucket = it.bucket->next;
        index = it.bucket->first_elm_idx;
    }
    it.elm = &it.bucket->elms[index];
    it.offset = &it.bucket->offsets[index] + 1;
    
    return it;
}

void sp_iter_go_next(sp_iter_t *it)
{
    sp_index_t index = it->offset->next_elm_index;
    
    if (SP_UNLIKELY(index == SP_BUCKET_SIZE))
    {
        it->bucket = it->bucket->next;
        index = it->bucket->first_elm_idx;
    }
    it->elm = &it->bucket->elms[index];
    it->offset = &it->bucket->offsets[index] + 1;
}

SP_TYPE *sp_iter_elm(sp_iter_t it)
{
    return &it.elm->value;
}

sp_iter_t sp_iter_pop(sp_iter_t it)
{
    SP_NAME *sp = it.sp;
    sp_bucket_t *bucket = it.bucket;
    sp_index_t index = it.elm - it.bucket->elms;
    sp_iter_t ret = {
        .sp = sp
    };
    if(SP_UNLIKELY(sp_bucket_pop(sp, bucket, index)))
    {
        sp_bucket_t *prev_bucket = sp_bucket_prev(sp, bucket);
        if(prev_bucket == NULL)
        {
            sp->buckets = bucket->next;
            sp->buckets->prev = NULL;
        }
        else
        {
            prev_bucket->next = bucket->next;
        }
        
        if(bucket == sp->tail)
        {
            if(prev_bucket != NULL)
            {
                sp->tail = prev_bucket;
            }
            else
            {
                sp->tail = sp->end_sentinel;
                sp->buckets = sp->end_sentinel; // this is actually not needed, because we already did `sp->buckets = bucket->next;`
            }
            
            ret = sp_iter_to(sp, sp->end_sentinel, SP_BUCKET_SIZE);
        }
        else
        {
            ret = sp_iter_to(sp, bucket->next, sp_bucket_first_elm(bucket->next));
        }
        
        SP_FREE(SP_ALLOC_CTX, bucket, sizeof(*bucket));
        sp->bucket_count -= 1;
    }
    else
    {
        sp_index_t next_elm = index + 1;
        next_elm = bucket->offsets[next_elm].next_elm_index;
        
        bool bucket_end = next_elm == SP_BUCKET_SIZE;
        
        bucket = bucket_end ? bucket->next : bucket;
        next_elm = bucket_end ? bucket->first_elm_idx : next_elm;
        
        ret = sp_iter_to(sp, bucket, next_elm);
    }
    sp->count -= 1;
    return ret;
}

void *sp_alloc_mem(void *ctx, size_t size, size_t alignment)
{
    (void)ctx, (void)alignment;
    return malloc(size);
}

void *sp_realloc_mem(void *ctx, void *ptr, size_t old_size, size_t new_size, size_t alignment)
{
    (void) ctx, (void) old_size, (void) alignment;
    return realloc(ptr, new_size);
}

void sp_free_mem(void *ctx, void *ptr, size_t size)
{
    (void)ctx, (void)size;
    free(ptr);
}

#endif

#undef SP_TYPE
#undef SP_NAME
#undef SP_IMPL
#undef SP_ALLOC
#undef SP_FREE
#undef SP_ALLOC_CTX
#undef SP_BUCKET_SIZE

#undef sp_index_t
#undef SP_INDEX_MAX

#undef sp_iter_t
#undef sp_bucket_t
#undef sp_entry_t
#undef sp_offset_entry_t
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
#undef sp_bucket_prev

#undef sp_iter_next
#undef sp_iter_elm
#undef sp_iter_pop
#undef sp_iter_eq
#undef sp_iter_is_end

#undef sp_alloc_mem
#undef sp_free_mem
