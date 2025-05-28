#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdalign.h>
#include <string.h>
#include <time.h>
#include <assert.h>

#if !defined(HIVE_TYPE) || !defined(HIVE_NAME)

    #error "HIVE_TYPE and HIVE_NAME must be defined"

#endif

#if !defined(HIVE_BUCKET_SIZE)

    #define HIVE_BUCKET_SIZE (UINT16_MAX - 1)

#endif

#if !defined(HIVE_ALLOC)

    #define HIVE_ALLOC   hive_alloc_mem
    #define HIVE_FREE    hive_free_mem
    #define HIVE_REALLOC hive_realloc_mem

#else

    #if !defined(HIVE_FREE) || !defined(HIVE_REALLOC)
        #error "If you define HIVE_ALLOC you must also define HIVE_FREE and HIVE_REALLOC"
    #elif !defined(HIVE_IMPL)
        #warning "Only define allocator macros when you also define HIVE_IMPL"
    #endif

#endif

#if !defined(HIVE_ALLOC_CTX)

    #define HIVE_ALLOC_CTX NULL

#endif

#if HIVE_BUCKET_SIZE > (UINT16_MAX)

    #define hive_index_t   uint32_t
    #define HIVE_INDEX_MAX UINT32_MAX

#elif HIVE_BUCKET_SIZE > (UINT8_MAX)

    #define hive_index_t   uint16_t
    #define HIVE_INDEX_MAX UINT16_MAX

#else

    #define hive_index_t   uint8_t
    #define HIVE_INDEX_MAX UINT8_MAX

#endif

#if defined(__GNUC__)

    #define HIVE_LIKELY(x)   __builtin_expect(x, 1)
    #define HIVE_UNLIKELY(x) __builtin_expect(x, 0)

#else

    #define HIVE_LIKELY(x)   x
    #define HIVE_UNLIKELY(x) x

#endif

#define HIVE_CAT_(a,b) a##b
#define HIVE_CAT(a,b) HIVE_CAT_(a,b)

#define hive_iter_t                HIVE_CAT(HIVE_NAME, _iter_t)
#define hive_bucket_t              HIVE_CAT(HIVE_NAME, _bucket_t)
#define hive_entry_t               HIVE_CAT(HIVE_NAME, _entry_t)
#define hive_next_entry_t          HIVE_CAT(HIVE_NAME, _next_entry_t)
#define hive_init                  HIVE_CAT(HIVE_NAME, _init)
#define hive_clone                 HIVE_CAT(HIVE_NAME, _clone)
#define hive_push_not_full_bucket  HIVE_CAT(HIVE_NAME, _push_not_full_bucket)
#define hive_put                   HIVE_CAT(HIVE_NAME, _put)
#define hive_put_all               HIVE_CAT(HIVE_NAME, _put_all)
#define hive_del_helper            HIVE_CAT(HIVE_NAME, _del_helper)
#define hive_del                   HIVE_CAT(HIVE_NAME, _del)
#define hive_foreach               HIVE_CAT(HIVE_NAME, _foreach)
#define hive_foreach_updater       HIVE_CAT(HIVE_NAME, _foreach_updater)
#define hive_begin                 HIVE_CAT(HIVE_NAME, _begin)
#define hive_end                   HIVE_CAT(HIVE_NAME, _end)
#define hive_deinit                HIVE_CAT(HIVE_NAME, _deinit)

#define hive_bucket_init           HIVE_CAT(HIVE_NAME, _bucket_init)
#define hive_bucket_put            HIVE_CAT(HIVE_NAME, _bucket_put)
#define hive_bucket_del            HIVE_CAT(HIVE_NAME, _bucket_del)
#define hive_bucket_is_elm_within  HIVE_CAT(HIVE_NAME, _bucket_is_elm_within)
#define hive_get_containing_bucket HIVE_CAT(HIVE_NAME, _get_containing_bucket)
#define hive_bucket_first_elm      HIVE_CAT(HIVE_NAME, _bucket_first_elm)
#define hive_bucket_prev           HIVE_CAT(HIVE_NAME, _bucket_prev)

#define hive_iter_next             HIVE_CAT(HIVE_NAME, _iter_next)
#define hive_iter_go_next          HIVE_CAT(HIVE_NAME, _iter_go_next)
#define hive_iter_elm              HIVE_CAT(HIVE_NAME, _iter_elm)
#define hive_iter_del              HIVE_CAT(HIVE_NAME, _iter_del)
#define hive_iter_eq               HIVE_CAT(HIVE_NAME, _iter_eq)
#define hive_iter_is_end           HIVE_CAT(HIVE_NAME, _iter_is_end)
#define hive_iter_to               HIVE_CAT(HIVE_NAME, _iter_to)
#define hive_validate              HIVE_CAT(HIVE_NAME, _validate)

#define hive_alloc_mem             HIVE_CAT(HIVE_NAME, _alloc_mem)
#define hive_realloc_mem           HIVE_CAT(HIVE_NAME, _realloc_mem)
#define hive_free_mem              HIVE_CAT(HIVE_NAME, _free_mem)

#define HIVE_ARR_LEN(arr) \
sizeof(arr) / sizeof(arr[0])

#define HIVE_GET_BUCKET_SIZE(hv) \
(HIVE_ARR_LEN((hv)->buckets->elms) - 1)

#define HIVE_FOREACH(hv, body)                                                                    \
do                                                                                                \
{                                                                                                 \
    typeof(*hv) * const hive_hv = hv;                                                             \
    const typeof(*hive_hv->buckets) *const hive_end_bucket = hive_hv->end_sentinel;               \
    typeof(hive_hv->buckets) hive_bucket = hive_hv->buckets;                                      \
    typeof(hive_bucket->next_entries[0].next_elm_index) hive_index = hive_bucket->first_elm_idx;  \
    typeof(hive_bucket->next_entries[0]) *hive_next_entries_base = &hive_bucket->next_entries[0]; \
    typeof(*hive_bucket->elms) *hive_elms_base = &hive_bucket->elms[0];                           \
    while( hive_bucket != hive_end_bucket )                                                       \
    {                                                                                             \
        { body }                                                                                  \
                                                                                                  \
        (hive_index) = hive_next_entries_base[hive_index + 1].next_elm_index;                     \
        if(HIVE_UNLIKELY(hive_index >= HIVE_GET_BUCKET_SIZE(hive_hv)))                            \
        {                                                                                         \
            (hive_bucket) = (hive_bucket)->next;                                                  \
            (hive_index) = (hive_bucket)->first_elm_idx;                                          \
            hive_next_entries_base = &hive_bucket->next_entries[0];                               \
            hive_elms_base = &hive_bucket->elms[0];                                               \
        }                                                                                         \
    }                                                                                             \
} while(0)

#define HIVE_ITER_ELM \
((typeof(hive_bucket->elms[0].value)*const) &hive_elms_base[hive_index])

#define HIVE_GET_ITER(itr) \
*(itr) = (typeof(*(itr))){.hv = hive_hv, .bucket = hive_bucket, .next_entry = hive_next_entries_base + hive_index + 1, .elm = &hive_elms_base[hive_index]}

#define HIVE_SET_ITER(itr)                            \
do {                                                  \
    const typeof(itr) hive_itr = itr;                 \
    hive_bucket = hive_itr.bucket;                    \
    hive_elms_base = &hive_bucket->elms[0];           \
    hive_next_entries_base = &hive_bucket->next_entries[0]; \
    hive_index = hive_itr.elm - hive_elms_base;       \
} while(0)

typedef struct hive_entry_t
{
    typeof(HIVE_TYPE) value;
} hive_entry_t;

typedef struct hive_next_entry_t
{
    hive_index_t next_elm_index;
} hive_next_entry_t;

typedef struct hive_bucket_t
{
    uint16_t not_full_idx; // if this bucket is not full, this will be set to its index inside the `not_full_buckets` array
    hive_index_t first_empty_idx;
    hive_index_t first_elm_idx;
    hive_index_t count;
    hive_entry_t elms[HIVE_BUCKET_SIZE + 1];
    hive_next_entry_t next_entries[HIVE_BUCKET_SIZE + 1];
    struct hive_bucket_t *next;
    struct hive_bucket_t *prev;
} hive_bucket_t;

typedef struct HIVE_NAME
{
    hive_bucket_t *buckets;
    hive_bucket_t *tail;
    hive_bucket_t *end_sentinel; // tail->next == end_sentinel
    struct
    {
        hive_bucket_t **array;
        uint16_t count;
        uint16_t cap;
    } not_full_buckets;
    size_t count;
    size_t bucket_count;
} HIVE_NAME;

typedef struct hive_iter_t
{
    HIVE_NAME *hv;
    hive_bucket_t *bucket;
    hive_next_entry_t *next_entry;
    hive_entry_t *elm;
} hive_iter_t;

void hive_init(HIVE_NAME *hv);
HIVE_NAME hive_clone(const HIVE_NAME *const hv);
hive_iter_t hive_put(HIVE_NAME *hv, HIVE_TYPE new_elm);
void hive_put_all(HIVE_NAME *hv, HIVE_TYPE *elms, size_t nelms);
hive_iter_t hive_del(HIVE_NAME *hv, HIVE_TYPE *elm);
void hive_foreach(const HIVE_NAME *hv, void(*f)(HIVE_TYPE*,void*), void *arg);
void hive_deinit(HIVE_NAME *hv);

hive_iter_t hive_begin(HIVE_NAME *hv);
hive_iter_t hive_end(HIVE_NAME *hv);
hive_iter_t hive_iter_next(hive_iter_t it);
void hive_iter_go_next(hive_iter_t *it);
HIVE_TYPE *hive_iter_elm(hive_iter_t it);
hive_iter_t hive_iter_del(hive_iter_t it);
bool hive_iter_eq(hive_iter_t a, hive_iter_t b);
bool hive_iter_is_end(hive_iter_t it);

#define HIVE_IMPL
#if defined(HIVE_IMPL)

void hive_foreach_updater(hive_index_t *index, hive_bucket_t **bucket);
void hive_bucket_init(hive_bucket_t *bucket);
void hive_push_not_full_bucket(HIVE_NAME *hv, hive_bucket_t *bucket);
HIVE_TYPE *hive_bucket_put(HIVE_NAME *hv, hive_bucket_t *bucket, HIVE_TYPE new_elm);
bool hive_bucket_del(HIVE_NAME *hv, hive_bucket_t *bucket, hive_index_t index);
hive_iter_t hive_del_helper(HIVE_NAME *hv, hive_bucket_t *prev_bucket, hive_bucket_t *bucket, hive_index_t index);
bool hive_bucket_is_elm_within(const hive_bucket_t *bucket, const HIVE_TYPE *elm);
hive_bucket_t *hive_get_containing_bucket(HIVE_NAME *hv, const HIVE_TYPE *elm);
hive_index_t hive_bucket_first_elm(hive_bucket_t *bucket);
hive_bucket_t *hive_bucket_prev(HIVE_NAME *hv, hive_bucket_t *bucket);
hive_iter_t hive_iter_to(HIVE_NAME *hv, hive_bucket_t *bucket, hive_index_t index);

void *hive_alloc_mem(void *ctx, size_t size, size_t alignment);
void *hive_realloc_mem(void *ctx, void *ptr, size_t old_size, size_t new_size, size_t alignment);
void hive_free_mem(void *ctx, void *ptr, size_t size);

#define HIVE_ALLOC_N(type, count) \
(typeof(type)*) HIVE_ALLOC(HIVE_ALLOC_CTX, sizeof(type) * count, alignof(type))

#define HIVE_REALLOC_N(ptr, old_count, new_count) \
(typeof(ptr)) HIVE_REALLOC(HIVE_ALLOC_CTX, (void*)(ptr), sizeof((ptr)[0]) * (old_count), sizeof((ptr)[0]) * (new_count), alignof(typeof((ptr)[0])))

#define HIVE_FREE_N(ptr, count) \
HIVE_FREE(HIVE_ALLOC_CTX, (void*)ptr, sizeof(ptr[0]) * count)

void hive_init(HIVE_NAME *hv)
{
    hive_bucket_t *end_sentinel = (hive_bucket_t*) HIVE_ALLOC(HIVE_ALLOC_CTX, sizeof(hive_bucket_t), alignof(hive_bucket_t));
    hive_bucket_init(end_sentinel);
    end_sentinel->first_elm_idx = HIVE_BUCKET_SIZE;
    (*hv) = (HIVE_NAME){
        .buckets      = end_sentinel,
        .tail         = end_sentinel,
        .end_sentinel = end_sentinel,
        .count        = 0,
        .bucket_count = 0,
    };
    hv->not_full_buckets.count = 0;
    hv->not_full_buckets.cap = 16;
    hv->not_full_buckets.array = HIVE_ALLOC_N(hive_bucket_t*, hv->not_full_buckets.cap);
}

HIVE_NAME hive_clone(const HIVE_NAME *const hv)
{
    HIVE_NAME ret;
    hive_init(&ret);
    ret.count = hv->count;
    ret.bucket_count = hv->bucket_count;
    
    if(hv->not_full_buckets.count > ret.not_full_buckets.cap)
    {
        ret.not_full_buckets.array = HIVE_REALLOC_N(ret.not_full_buckets.array, ret.not_full_buckets.cap, hv->not_full_buckets.count + 1);
        ret.not_full_buckets.cap = hv->not_full_buckets.count + 1;
    }
    
    ret.not_full_buckets.count = hv->not_full_buckets.count;
    
    if(hv->bucket_count > 0)
    {
        hive_bucket_t *src_bucket = hv->buckets;
        
        hive_bucket_t **dst_bucket = &ret.buckets;
        hive_bucket_t *dst_prev = NULL;
        
        *dst_bucket = (hive_bucket_t*) HIVE_ALLOC(HIVE_ALLOC_CTX, sizeof(hive_bucket_t), alignof(hive_bucket_t));
        memcpy(*dst_bucket, src_bucket, sizeof(hive_bucket_t));
        (*dst_bucket)->prev = NULL;
        
        if((*dst_bucket)->not_full_idx != UINT16_MAX)
        {
            ret.not_full_buckets.array[(*dst_bucket)->not_full_idx] = *dst_bucket;
        }
        
        (*dst_bucket)->next = NULL;
        dst_prev = (*dst_bucket);
        dst_bucket = &(*dst_bucket)->next;
        src_bucket = src_bucket->next;
        
        while(src_bucket != hv->end_sentinel)
        {
            *dst_bucket = (hive_bucket_t*) HIVE_ALLOC(HIVE_ALLOC_CTX, sizeof(hive_bucket_t), alignof(hive_bucket_t));
            memcpy(*dst_bucket, src_bucket, sizeof(hive_bucket_t));
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

hive_iter_t hive_put(HIVE_NAME *hv, HIVE_TYPE new_elm)
{
    HIVE_TYPE *elm_added = NULL;
    
    if(hv->not_full_buckets.count > 0)
    {
        hive_bucket_t *bucket = hv->not_full_buckets.array[hv->not_full_buckets.count - 1];
        hv->count += 1;
        HIVE_TYPE *elm = hive_bucket_put(hv, bucket, new_elm);
        return hive_iter_to(hv, bucket, elm - &bucket->elms[0].value);
    }
    
    hive_bucket_t *new_bucket = (hive_bucket_t*) HIVE_ALLOC(HIVE_ALLOC_CTX, sizeof(hive_bucket_t), alignof(hive_bucket_t));
    hive_bucket_init(new_bucket);
    hive_push_not_full_bucket(hv, new_bucket);
    
    if(hv->count > 0)
    {
        hv->tail->next = new_bucket;
        new_bucket->prev = hv->tail;
        new_bucket->next = hv->end_sentinel;
        hv->tail = new_bucket;
    }
    else
    {
         hv->buckets = new_bucket;
         hv->tail = new_bucket;
         new_bucket->next = hv->end_sentinel;
    }
    
    hv->end_sentinel->prev = new_bucket;
    hv->bucket_count += 1;
    elm_added = hive_bucket_put(hv, new_bucket, new_elm);
    hv->count += 1;
    return hive_iter_to(hv, new_bucket, elm_added - &new_bucket->elms[0].value);
}

void hive_put_all(HIVE_NAME *hv, HIVE_TYPE *elms, size_t nelms)
{
    size_t buckets_to_fill = nelms / HIVE_BUCKET_SIZE;
    ptrdiff_t remaining = nelms - (buckets_to_fill * HIVE_BUCKET_SIZE);
    
    hive_bucket_t *first = NULL;
    bool first_set = false;
    hive_bucket_t *bucket = NULL;
    hive_bucket_t *prev = NULL;
    for(size_t i = 0 ; i < buckets_to_fill ; i++)
    {
        bucket = (hive_bucket_t*) HIVE_ALLOC(HIVE_ALLOC_CTX, sizeof(hive_bucket_t), alignof(hive_bucket_t));
        if(!first_set)
        {
            first = bucket;
            first_set = true;
        }
        hive_bucket_init(bucket);
        memcpy(bucket->elms, elms + (i * HIVE_BUCKET_SIZE), HIVE_BUCKET_SIZE * sizeof(HIVE_TYPE));
        for(hive_index_t j = 0 ; j <= HIVE_BUCKET_SIZE ; j++)
        {
            bucket->next_entries[j].next_elm_index = j;
        }
        bucket->first_elm_idx = 0;
        bucket->count = HIVE_BUCKET_SIZE;
        hv->bucket_count += 1;
        hv->count += HIVE_BUCKET_SIZE;
        
        bucket->prev = prev;
        if(prev)
        {
            prev->next = bucket;
        }
        prev = bucket;
    }
    
    if(remaining > 0)
    {
        hive_bucket_t *remaining_bucket = (hive_bucket_t*) HIVE_ALLOC(HIVE_ALLOC_CTX, sizeof(hive_bucket_t), alignof(hive_bucket_t));;
        hive_bucket_init(remaining_bucket);
        memcpy(remaining_bucket->elms, elms + (buckets_to_fill * HIVE_BUCKET_SIZE), remaining * sizeof(HIVE_TYPE));
        for(hive_index_t j = 0 ; j < remaining ; j++)
        {
            remaining_bucket->next_entries[j].next_elm_index = j;
        }
        remaining_bucket->first_elm_idx = 0;
        remaining_bucket->first_empty_idx = remaining;
        remaining_bucket->count = remaining;
        
        hive_push_not_full_bucket(hv, remaining_bucket);
        
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
        
        hv->count += remaining;
        hv->bucket_count += 1;
    }
    
    if(hv->buckets != hv->end_sentinel && first != NULL)
    {
        hv->tail->next = first;
        hv->tail = bucket;
        hv->tail->next = hv->end_sentinel;
        hv->end_sentinel->prev = hv->tail;
    }
    else
    {
        hv->buckets = first;
        hv->tail = bucket;
        hv->tail->next = hv->end_sentinel;
        hv->end_sentinel->prev = hv->tail;
    }
}

hive_iter_t hive_del_helper(HIVE_NAME *hv, hive_bucket_t *prev_bucket, hive_bucket_t *bucket, hive_index_t index)
{
    hive_iter_t ret = {
        .hv = hv
    };
    if(hive_bucket_del(hv, bucket, index))
    {
        if(prev_bucket != NULL)
        {
            prev_bucket->next = bucket->next;
        }
        else
        {
            hv->buckets = bucket->next;
        }
        if(bucket->next != NULL)
        {
            bucket->next->prev = prev_bucket;
        }
        
        if(bucket == hv->tail)
        {
            if(prev_bucket != NULL)
            {
                hv->tail = prev_bucket;
            }
            else
            {
                hv->tail = hv->end_sentinel;
                hv->end_sentinel->prev = NULL;
            }
            
            ret = hive_iter_to(hv, hv->end_sentinel, HIVE_BUCKET_SIZE);
        }
        else
        {
            hive_index_t first_elm = hive_bucket_first_elm(bucket->next);
            ret = hive_iter_to(hv, bucket->next, first_elm);
        }
        
        HIVE_FREE(HIVE_ALLOC_CTX, bucket, sizeof(*bucket));
        hv->bucket_count -= 1;
    }
    else
    {
        hive_index_t next_elm = bucket->next_entries[index + 1].next_elm_index;
        
        if(next_elm == HIVE_BUCKET_SIZE)
        {
            hive_bucket_t *next_bucket = bucket->next;
            hive_index_t first_elm_in_next_bucket = hive_bucket_first_elm(next_bucket);
            ret = hive_iter_to(hv, next_bucket, first_elm_in_next_bucket);
        }
        else
        {
            ret = hive_iter_to(hv, bucket, next_elm);
        }
    }
    hv->count -= 1;
    return ret;
}


hive_iter_t hive_del(HIVE_NAME *hv, HIVE_TYPE *elm)
{
    hive_bucket_t *prev = NULL;
    hive_iter_t ret = {
        .hv = hv
    };
    for(hive_bucket_t *bucket = hv->buckets ; bucket != hv->end_sentinel ; prev = bucket, bucket = bucket->next)
    {
        if(hive_bucket_is_elm_within(bucket, elm))
        {
            hive_entry_t *as_entry = (hive_entry_t*) elm;
            hive_index_t index = as_entry - bucket->elms;
            ret = hive_del_helper(hv, prev, bucket, index);
            break;
        }
    }
    return ret;
}

void hive_foreach_updater(hive_index_t *index, hive_bucket_t **bucket)
{
    ++(*index);
    (*index) = (*bucket)->next_entries[*index].next_elm_index;
    if(*index == HIVE_BUCKET_SIZE)
    {
        (*bucket) = (*bucket)->next;
        (*index) = (*bucket)->first_elm_idx;
    }
}

void hive_foreach(const HIVE_NAME *hv, void(*f)(HIVE_TYPE*,void*), void *arg)
{
    hive_bucket_t *bucket = hv->buckets;
    hive_index_t index = bucket->first_elm_idx;
    for( ; bucket != hv->end_sentinel ; hive_foreach_updater(&index, &bucket))
    {
        f(&bucket->elms[index].value, arg);
    }
}

void hive_deinit(HIVE_NAME *hv)
{
    for(hive_bucket_t *current = hv->buckets ; current != NULL ; )
    {
        hive_bucket_t *next = current->next;
        HIVE_FREE(HIVE_ALLOC_CTX, current, sizeof(hive_bucket_t));
        current = next;
    }
    HIVE_FREE_N(hv->not_full_buckets.array, hv->not_full_buckets.count);
    *hv = (HIVE_NAME){0};
}

void hive_bucket_init(hive_bucket_t *bucket)
{
    bucket->first_elm_idx = HIVE_BUCKET_SIZE;
    bucket->next = NULL;
    bucket->prev = NULL;
    bucket->not_full_idx = UINT16_MAX;
    bucket->count = 0;
    
    hive_index_t i;
    for(i = 0 ; i < HIVE_BUCKET_SIZE ; i++)
    {
        bucket->next_entries[i].next_elm_index = HIVE_BUCKET_SIZE;
    }
    bucket->next_entries[HIVE_BUCKET_SIZE].next_elm_index = HIVE_BUCKET_SIZE;
    
    bucket->first_empty_idx = 0;
}

void hive_push_not_full_bucket(HIVE_NAME *hv, hive_bucket_t *bucket)
{
    if(hv->not_full_buckets.cap <= hv->not_full_buckets.count)
    {
        uint16_t new_cap = hv->not_full_buckets.cap * 2;
        hv->not_full_buckets.array = HIVE_REALLOC_N(hv->not_full_buckets.array, hv->not_full_buckets.cap, new_cap);
        hv->not_full_buckets.cap = new_cap;
    }
    
    hv->not_full_buckets.array[hv->not_full_buckets.count] = bucket;
    bucket->not_full_idx = hv->not_full_buckets.count;
    
    hv->not_full_buckets.count += 1;
}

HIVE_TYPE *hive_bucket_put(HIVE_NAME *hv, hive_bucket_t *bucket, HIVE_TYPE new_elm)
{
    assert(bucket->count < HIVE_BUCKET_SIZE);
    
    int empty_index = bucket->first_empty_idx;
    
    assert(bucket->next_entries[empty_index].next_elm_index != empty_index);
    
    bucket->elms[empty_index].value = new_elm;
    bucket->next_entries[empty_index].next_elm_index = empty_index;
    bucket->count += 1;
    
    if(empty_index < bucket->first_elm_idx)
    {
        bucket->first_elm_idx = empty_index;
    }
    
    if(bucket->count != HIVE_BUCKET_SIZE)
    {
        hive_index_t next_empty;
        for(next_empty = bucket->first_empty_idx + 1 ; bucket->next_entries[next_empty].next_elm_index == next_empty ; next_empty++)
            ;
        bucket->first_empty_idx = next_empty;
    }
    else
    {
        bucket->first_empty_idx = HIVE_BUCKET_SIZE;
        bucket->not_full_idx = UINT16_MAX;
        hv->not_full_buckets.count -= 1;
    }
    return &bucket->elms[empty_index].value;
}

bool hive_bucket_del(HIVE_NAME *hv, hive_bucket_t *bucket, hive_index_t index)
{
    (void)hv;
    assert(bucket->next_entries[index].next_elm_index == index);
    assert(bucket->count != 0);
    
    bucket->next_entries[index].next_elm_index = bucket->next_entries[index + 1].next_elm_index;
    
    for(ptrdiff_t i = (ptrdiff_t)index - 1 ; i >= 0 && bucket->next_entries[i].next_elm_index != i ; i--)
    {
        bucket->next_entries[i].next_elm_index = bucket->next_entries[index].next_elm_index;
    }
    
    if(HIVE_UNLIKELY(index == bucket->first_elm_idx))
    {
        hive_index_t next_elm = bucket->next_entries[index].next_elm_index;
        bucket->first_elm_idx = next_elm;
        
        if(next_elm == HIVE_BUCKET_SIZE)
        {
            hv->not_full_buckets.array[bucket->not_full_idx] = hv->not_full_buckets.array[hv->not_full_buckets.count-1];
            hv->not_full_buckets.array[bucket->not_full_idx]->not_full_idx = bucket->not_full_idx;
            hv->not_full_buckets.count -= 1;
            
            bucket->count -= 1;
            return true;
        }
    }
    
    if(HIVE_UNLIKELY(bucket->count == HIVE_BUCKET_SIZE))
    {
        hive_push_not_full_bucket(hv, bucket);
        bucket->first_empty_idx = index;
    }
    else if(index < bucket->first_empty_idx)
    {
        bucket->first_empty_idx = index;
    }
    
    bucket->count -= 1;
    
    return false;
}

hive_index_t hive_bucket_first_elm(hive_bucket_t *bucket)
{
    return bucket->first_elm_idx;
}

hive_bucket_t *hive_bucket_prev(HIVE_NAME *hv, hive_bucket_t *bucket)
{
    return bucket->prev;
}

bool hive_bucket_is_elm_within(const hive_bucket_t *bucket, const HIVE_TYPE *elm)
{
    uintptr_t
    ibegin = (uintptr_t) (bucket->elms),
    iend   = (uintptr_t) (bucket->elms + HIVE_BUCKET_SIZE),
    ielm   = (uintptr_t) (elm);
    return ielm >= ibegin && ielm < iend;
}

hive_bucket_t *hive_get_containing_bucket(HIVE_NAME *hv, const HIVE_TYPE *elm)
{
    hive_bucket_t *bucket = hv->buckets;
    while(bucket != NULL)
    {
        if(hive_bucket_is_elm_within(bucket, elm))
        {
            return bucket;
        }
        bucket = bucket->next;
    }
    return NULL;
}

hive_iter_t hive_begin(HIVE_NAME *hv)
{
    return hive_iter_to(hv, hv->buckets, hv->buckets->first_elm_idx);
}

hive_iter_t hive_end(HIVE_NAME *hv)
{
    return hive_iter_to(hv, hv->end_sentinel, HIVE_BUCKET_SIZE);
}

hive_iter_t hive_iter_to(HIVE_NAME *hv, hive_bucket_t *bucket, hive_index_t index)
{
    hive_iter_t ret;
    ret.hv = hv;
    ret.bucket = bucket;
    ret.elm = &bucket->elms[index];
    ret.next_entry = &ret.bucket->next_entries[index] + 1;
    return ret;
}

bool hive_iter_eq(hive_iter_t a, hive_iter_t b)
{
    return (a.elm == b.elm);
}

bool hive_iter_is_end(hive_iter_t it)
{
    return (it.bucket == it.hv->end_sentinel);
}

hive_iter_t hive_iter_next(hive_iter_t it)
{
    hive_index_t index = it.next_entry->next_elm_index;
    
    if (HIVE_UNLIKELY(index == HIVE_BUCKET_SIZE))
    {
        it.bucket = it.bucket->next;
        index = it.bucket->first_elm_idx;
    }
    it.elm = &it.bucket->elms[index];
    it.next_entry = &it.bucket->next_entries[index] + 1;
    
    return it;
}

void hive_iter_go_next(hive_iter_t *it)
{
    hive_index_t index = it->next_entry->next_elm_index;
    
    if (HIVE_UNLIKELY(index == HIVE_BUCKET_SIZE))
    {
        it->bucket = it->bucket->next;
        index = it->bucket->first_elm_idx;
    }
    it->elm = &it->bucket->elms[index];
    it->next_entry = &it->bucket->next_entries[index] + 1;
}

HIVE_TYPE *hive_iter_elm(hive_iter_t it)
{
    return &it.elm->value;
}

hive_iter_t hive_iter_del(hive_iter_t it)
{
    HIVE_NAME *hv = it.hv;
    hive_bucket_t *bucket = it.bucket;
    hive_index_t index = it.elm - it.bucket->elms;
    
    return hive_del_helper(it.hv, it.bucket->prev, it.bucket, index);
}

bool hive_validate(HIVE_NAME *hive)
{
    hive_bucket_t *b = hive->buckets;
    while(b != hive->end_sentinel)
    {
        hive_index_t begin = 0;
        hive_index_t next_elm = b->first_elm_idx;
        while(begin != HIVE_BUCKET_SIZE)
        {
            for(hive_index_t i = begin ; i <= next_elm ; i++)
            {
                assert(b->next_entries[i].next_elm_index == next_elm);
            }
            
            begin = next_elm + 1;
            // find next elm
            bool set = false;
            if(next_elm == HIVE_BUCKET_SIZE)
            {
                set = true;
                begin = HIVE_BUCKET_SIZE;
            }
            else
                for(hive_index_t i = next_elm + 1 ; i <= HIVE_BUCKET_SIZE ; i++)
                {
                    if(b->next_entries[i].next_elm_index == i)
                    {
                        next_elm = i;
                        set = true;
                        break;
                    }
                }
            assert(set);
        }
        
        b = b->next;
    }
}

void *hive_alloc_mem(void *ctx, size_t size, size_t alignment)
{
    (void)ctx, (void)alignment;
    return malloc(size);
}

void *hive_realloc_mem(void *ctx, void *ptr, size_t old_size, size_t new_size, size_t alignment)
{
    (void) ctx, (void) old_size, (void) alignment;
    return realloc(ptr, new_size);
}

void hive_free_mem(void *ctx, void *ptr, size_t size)
{
    (void)ctx, (void)size;
    free(ptr);
}

#endif

#undef HIVE_TYPE
#undef HIVE_NAME
#undef HIVE_IMPL
#undef HIVE_ALLOC
#undef HIVE_FREE
#undef HIVE_ALLOC_CTX
#undef HIVE_BUCKET_SIZE

#undef hive_index_t
#undef HIVE_INDEX_MAX

#undef hive_iter_t
#undef hive_bucket_t
#undef hive_entry_t
#undef hive_next_entry_t
#undef hive_init
#undef hive_clone
#undef hive_push_not_full_bucket
#undef hive_put
#undef hive_put_all
#undef hive_del_helper
#undef hive_del
#undef hive_foreach
#undef hive_foreach_updater
#undef hive_begin
#undef hive_end
#undef hive_deinit
#undef hive_bucket_init
#undef hive_bucket_put
#undef hive_bucket_del
#undef hive_bucket_is_elm_within
#undef hive_get_containing_bucket
#undef hive_bucket_first_elm
#undef hive_bucket_prev
#undef hive_iter_next
#undef hive_iter_go_next
#undef hive_iter_elm
#undef hive_iter_del
#undef hive_iter_eq
#undef hive_iter_is_end
#undef hive_iter_to
#undef hive_alloc_mem
#undef hive_realloc_mem
#undef hive_free_mem
