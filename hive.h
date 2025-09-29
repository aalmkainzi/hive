#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdalign.h>
#include <string.h>
#include <time.h>
#include <assert.h>

#ifdef _MSC_VER
#include <intrin.h>
#endif

#if !defined(HIVE_TYPE) || !defined(HIVE_NAME)

#error "HIVE_TYPE and HIVE_NAME must be defined"

#endif

#define HIVE_BUCKET_SIZE (256 - 2)
#define HIVE_END_SENTINEL_INDEX (HIVE_BUCKET_SIZE + 1)

#if !defined(HIVE_ALLOC)

#define HIVE_ALLOC   hive_alloc_mem
#define HIVE_FREE    hive_free_mem
#define HIVE_REALLOC hive_realloc_mem

#elif !defined(HIVE_IMPL)
#warning "Only define allocator macros when you also define HIVE_IMPL. This header will #undef your macros"
#endif

#if defined(HIVE_ALLOC)   && (!defined(HIVE_FREE)  || !defined(HIVE_REALLOC)) || \
    defined(HIVE_FREE)    && (!defined(HIVE_ALLOC) || !defined(HIVE_REALLOC)) || \
    defined(HIVE_REALLOC) && (!defined(HIVE_ALLOC) || !defined(HIVE_FREE)   )
#error "If one allocation macro is defined, the other two must be defined."
#endif

#if !defined(HIVE_ALLOC_CTX)

#define HIVE_ALLOC_CTX NULL

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

#define hive_iter           HIVE_CAT(HIVE_NAME, _iter)
#define hive_handle         HIVE_CAT(HIVE_NAME, _handle)
#define hive_bucket_t       HIVE_CAT(HIVE_NAME, _bucket_t)
#define hive_allocation_t   HIVE_CAT(HIVE_NAME, _allocation_t)
#define hive_next_entry_t   HIVE_CAT(HIVE_NAME, _next_entry_t)
#define hive_init           HIVE_CAT(HIVE_NAME, _init)
#define hive_clone          HIVE_CAT(HIVE_NAME, _clone)
#define hive_put            HIVE_CAT(HIVE_NAME, _put)
#define hive_put_uninit     HIVE_CAT(HIVE_NAME, _put_uninit)
#define hive_put_all        HIVE_CAT(HIVE_NAME, _put_all)
#define hive_del            HIVE_CAT(HIVE_NAME, _del)
#define hive_foreach        HIVE_CAT(HIVE_NAME, _foreach)
#define hive_begin          HIVE_CAT(HIVE_NAME, _begin)
#define hive_end            HIVE_CAT(HIVE_NAME, _end)
#define hive_deinit         HIVE_CAT(HIVE_NAME, _deinit)

#define hive_iter_next      HIVE_CAT(HIVE_NAME, _iter_next)
#define hive_iter_del       HIVE_CAT(HIVE_NAME, _iter_del)
#define hive_iter_eq        HIVE_CAT(HIVE_NAME, _iter_eq)
#define hive_ptr_to_iter    HIVE_CAT(HIVE_NAME, _ptr_to_iter)

#define hive_handle_del     HIVE_CAT(HIVE_NAME, _handle_del)
#define hive_iter_to_handle HIVE_CAT(HIVE_NAME, _iter_to_handle)
#define hive_ptr_to_handle  HIVE_CAT(HIVE_NAME, _ptr_to_handle)
#define hive_handle_to_iter HIVE_CAT(HIVE_NAME, _handle_to_iter)

#define HIVE_ARR_LEN(arr) \
sizeof(arr) / sizeof(arr[0])

#define HIVE_TYPEOF __typeof__

#define hive_entry_t HIVE_TYPEOF(HIVE_TYPE)

#if !defined(HIVE_DECLARED)

#define HIVE__FOR_EACH_NEXT(it) \
((it).next_entry->next_elm_index == HIVE_END_SENTINEL_INDEX ? \
(HIVE_TYPEOF((it))){ \
    .bucket = (it).bucket->next, \
    .next_entry = &(it).bucket->next->next_entries[hive_bitset_first_elm(&(it).bucket->next->empty_bitset)] + 1, \
    .ptr = &(it).bucket->next->elms[hive_bitset_first_elm(&(it).bucket->next->empty_bitset)] \
} : \
(HIVE_TYPEOF((it))){ \
    .bucket = (it).bucket, \
    .next_entry = &(it).bucket->next_entries[(it).next_entry->next_elm_index] + 1, \
    .ptr = &(it).bucket->elms[(it).next_entry->next_elm_index] \
} \
)

#define HIVE_FOR_EACH(name, from_it, to_it) \
for(HIVE_TYPEOF(from_it) name = (from_it) ; name.ptr != (to_it).ptr ; name = HIVE__FOR_EACH_NEXT(name))

typedef struct hive_allocation_t
{
    void *ptr;
    size_t size;
} hive_allocation_t;

typedef struct HIVE_NAME
{
    struct hive_bucket_t *buckets;
    struct hive_bucket_t *tail;
    struct hive_bucket_t *end_sentinel; // tail->next == end_sentinel
    struct
    {
        struct hive_bucket_t **array;
        size_t count;
        size_t cap;
    } bucket_reserve;
    struct
    {
        hive_allocation_t *array;
        size_t count;
        size_t cap;
    } allocations;
    struct
    {
        struct hive_bucket_t **array;
        uint16_t count;
        uint16_t cap;
    } not_full_buckets;
    size_t count;
    size_t bucket_count;
} HIVE_NAME;

typedef struct hive_iter
{
    struct hive_bucket_t *bucket;
    struct hive_next_entry_t *next_entry;
    hive_entry_t *ptr;
} hive_iter;

typedef struct hive_handle
{
    struct hive_bucket_t *bucket;
    hive_entry_t *ptr;
} hive_handle;

#endif // HIVE_DECLARED

#ifndef HIVE_COMMON_UTILS
#define HIVE_COMMON_UTILS

static void *hive_alloc_mem(void *ctx, size_t size, size_t alignment)
{
    (void)ctx, (void)alignment;
    return malloc(size);
}

static void *hive_realloc_mem(void *ctx, void *ptr, size_t old_size, size_t new_size, size_t alignment)
{
    (void) ctx, (void) old_size, (void) alignment;
    return realloc(ptr, new_size);
}

static void hive_free_mem(void *ctx, void *ptr, size_t size)
{
    (void)ctx, (void)size;
    free(ptr);
}

static inline uint8_t hive_ctz(uint32_t i)
{
#ifdef _MSC_VER
    unsigned long index;
    _BitScanForward(&index, i);
    return (uint8_t)index;
#else
    return (uint8_t)__builtin_ctz(i);
#endif
}

static inline uint8_t hive_ctz64(uint64_t i)
{
#ifdef _MSC_VER
    unsigned long index;
    _BitScanForward64(&index, i);
    return (uint8_t)index;
#else
    return (uint8_t)__builtin_ctzll(i);
#endif
}

static inline uint8_t hive_bitset_first_elm(const uint64_t (*_bitset)[4])
{
    const uint64_t _inv0 = ~((*_bitset)[0] | 1ULL);
    const uint64_t _inv1 = ~((*_bitset)[1]);
    const uint64_t _inv2 = ~((*_bitset)[2]);
    const uint64_t _inv3 = ~((*_bitset)[3]);
    
    const unsigned int _mask =
        (_inv0 != 0)        |
        ((_inv1 != 0) << 1) |
        ((_inv2 != 0) << 2) |
        ((_inv3 != 0) << 3);
    
    const uint64_t _invs[] = {_inv0, _inv1, _inv2, _inv3};
    
    const uint8_t _inv_idx = hive_ctz(_mask);
    return hive_ctz64(_invs[_inv_idx]) + (64 * _inv_idx);
}

static inline uint8_t hive_bitset_set_first_empty(uint64_t (*_bitset)[4])
{
    const unsigned int _mask =
        ( (*_bitset)[0] != 0)       |
        (((*_bitset)[1] != 0) << 1) |
        (((*_bitset)[2] != 0) << 2) |
        (((*_bitset)[3] != 0) << 3);
    
    const uint8_t _idx = hive_ctz(_mask);
    const uint8_t _bit_idx = hive_ctz64((*_bitset)[_idx]);
    (*_bitset)[_idx] &= (*_bitset)[_idx] - 1;
    return _bit_idx + (64 * _idx);
}

#endif

void hive_init(HIVE_NAME *_hv);
HIVE_NAME hive_clone(const HIVE_NAME *_hv);
hive_iter hive_put(HIVE_NAME *hv, hive_entry_t _new_elm);
hive_iter hive_put_uninit(HIVE_NAME *_hv);
void hive_put_all(HIVE_NAME *_hv, const hive_entry_t *_elms, size_t _nelms);
hive_iter hive_del(HIVE_NAME *_hv, hive_entry_t *_elm);
void hive_foreach(HIVE_NAME *_hv, void(*_f)(hive_entry_t*,void*), void *_arg);
void hive_deinit(HIVE_NAME *_hv);

hive_iter hive_begin(const HIVE_NAME *_hv);
hive_iter hive_end(const HIVE_NAME *_hv);
hive_iter hive_iter_next(hive_iter _it);
hive_iter hive_iter_del(HIVE_NAME *_hv, hive_iter _it);
bool hive_iter_eq(hive_iter _a, hive_iter _b);
hive_iter hive_ptr_to_iter(HIVE_NAME *_hive, hive_entry_t *_ptr); // O(bucket_count)

hive_handle hive_iter_to_handle(hive_iter _it);
hive_iter hive_handle_del(HIVE_NAME *_hive, hive_handle _handle);
hive_handle hive_ptr_to_handle(HIVE_NAME *_hive, hive_entry_t *_ptr); // O(bucket_count)
hive_iter hive_handle_to_iter(hive_handle _handle);

#if defined(HIVE_IMPL) || defined(HIVE_IMPL_ALL)

typedef struct hive_next_entry_t
{
    uint8_t next_elm_index;
} hive_next_entry_t;

typedef struct hive_bucket_t
{
    struct hive_bucket_t *next;
    struct hive_bucket_t *prev;
    
    uint64_t empty_bitset[4]; // 1 = empty
    
    uint16_t not_full_idx; // if this bucket is not full, this will be set to its index inside the `not_full_buckets` array
    uint8_t count;
    hive_entry_t *elms;
    hive_next_entry_t *next_entries;
    hive_next_entry_t *prev_entries;
} hive_bucket_t;

#define hive_allocate_buckets            HIVE_CAT(HIVE_NAME, _allocate_buckets)
#define hive_push_allocation             HIVE_CAT(HIVE_NAME, _push_allocation)
#define hive_push_to_buckets_reserve     HIVE_CAT(HIVE_NAME, _push_to_buckets_reserve)
#define hive_push_used_bucket_to_reserve HIVE_CAT(HIVE_NAME, _push_used_bucket_to_reserve)
#define hive_push_not_full_bucket        HIVE_CAT(HIVE_NAME, _push_not_full_bucket)
#define hive_foreach_updater             HIVE_CAT(HIVE_NAME, _foreach_updater)

#define hive_bucket_init                 HIVE_CAT(HIVE_NAME, _bucket_init)
#define hive_bucket_reserve_slot         HIVE_CAT(HIVE_NAME, _bucket_reserve_slot)
#define hive_bucket_del                  HIVE_CAT(HIVE_NAME, _bucket_del)
#define hive_bucket_is_elm_within        HIVE_CAT(HIVE_NAME, _bucket_is_elm_within)
#define hive_bucket_first_elm            HIVE_CAT(HIVE_NAME, _bucket_first_elm)
#define hive_del_helper                  HIVE_CAT(HIVE_NAME, _del_helper)

#define hive_get_iter_from_index         HIVE_CAT(HIVE_NAME, _iter_to)

#ifdef HIVE_TEST
    #define hive_checked_put         HIVE_CAT(HIVE_NAME, _checked_put)
    #define hive_checked_del         HIVE_CAT(HIVE_NAME, _checked_del)
    #define hive_checked_iter_del    HIVE_CAT(HIVE_NAME, _iter_checked_del)

    hive_iter hive_checked_put(HIVE_NAME *_hive, HIVE_TYPE elm);
    hive_iter hive_checked_del(HIVE_NAME *_hive, HIVE_TYPE *elm);
    hive_iter hive_checked_iter_del(HIVE_NAME *_hive, hive_iter it);
#endif

void hive_foreach_updater(uint8_t *_index, hive_bucket_t **_bucket);
void hive_bucket_init(hive_bucket_t *_bucket);
void hive_push_not_full_bucket(HIVE_NAME *_hv, hive_bucket_t *_bucket);
HIVE_TYPE *hive_bucket_reserve_slot(HIVE_NAME *_hv, hive_bucket_t *_bucket);
bool hive_bucket_del(HIVE_NAME *_hv, hive_bucket_t *_bucket, uint8_t _index);
hive_iter hive_del_helper(HIVE_NAME *_hv, hive_bucket_t *_prev_bucket, hive_bucket_t *_bucket, uint8_t _index);
bool hive_bucket_is_elm_within(const hive_bucket_t *_bucket, const HIVE_TYPE *_elm);
uint8_t hive_bucket_first_elm(const hive_bucket_t *_bucket);
hive_iter hive_get_iter_from_index(hive_bucket_t *_bucket, uint8_t _index);
void hive_push_allocation(HIVE_NAME *_hv, void *_a, size_t _size);
void hive_push_to_buckets_reserve(HIVE_NAME *_hv, hive_bucket_t *_new_buckets, size_t _nb_buckets);
void hive_push_used_bucket_to_reserve(HIVE_NAME *_hv, hive_bucket_t *_bucket);
hive_bucket_t *hive_allocate_buckets(HIVE_NAME *_hv, size_t _nb);

void *hive_alloc_mem(void *_ctx, size_t _size, size_t _alignment);
void *hive_realloc_mem(void *_ctx, void *_ptr, size_t _old_size, size_t _new_size, size_t _alignment);
void hive_free_mem(void *_ctx, void *_ptr, size_t _size);

#define HIVE_ALLOC_N(type, count) \
(type*) HIVE_ALLOC(HIVE_ALLOC_CTX, sizeof(type) * (count), alignof(type))

#define HIVE_REALLOC_N(ptr, old_count, new_count) \
(HIVE_TYPEOF(ptr)) HIVE_REALLOC(HIVE_ALLOC_CTX, (void*)(ptr), sizeof((ptr)[0]) * (old_count), sizeof((ptr)[0]) * (new_count), alignof(HIVE_TYPEOF((ptr)[0])))

#define HIVE_FREE_N(ptr, count) \
HIVE_FREE(HIVE_ALLOC_CTX, (void*)(ptr), sizeof(ptr[0]) * (count))

void hive_init(HIVE_NAME *_hv)
{
    hive_bucket_t *_end_sentinel = (hive_bucket_t*) HIVE_ALLOC(HIVE_ALLOC_CTX, sizeof(hive_bucket_t), alignof(hive_bucket_t));
    _end_sentinel->next_entries = HIVE_ALLOC_N(hive_next_entry_t, HIVE_BUCKET_SIZE + 2);
    _end_sentinel->prev_entries = HIVE_ALLOC_N(hive_next_entry_t, HIVE_BUCKET_SIZE + 2);
    _end_sentinel->elms = HIVE_ALLOC_N(hive_entry_t, HIVE_BUCKET_SIZE + 2);
    hive_bucket_init(_end_sentinel);
    
    _hv->buckets      = _end_sentinel,
    _hv->tail         = _end_sentinel,
    _hv->end_sentinel = _end_sentinel,
    _hv->count        = 0,
    _hv->bucket_count = 0,
    
    _hv->not_full_buckets.cap = 16;
    _hv->not_full_buckets.count = 0;
    _hv->not_full_buckets.array = HIVE_ALLOC_N(hive_bucket_t*, _hv->not_full_buckets.cap);
    
    _hv->allocations.cap = 16;
    _hv->allocations.count = 0;
    _hv->allocations.array = HIVE_ALLOC_N(hive_allocation_t, _hv->allocations.cap);
    
    hive_push_allocation(_hv, _end_sentinel, sizeof(hive_bucket_t));
    hive_push_allocation(_hv, _end_sentinel->next_entries, sizeof(hive_next_entry_t) * (HIVE_BUCKET_SIZE + 2));
    hive_push_allocation(_hv, _end_sentinel->prev_entries, sizeof(hive_next_entry_t) * (HIVE_BUCKET_SIZE + 2));
    hive_push_allocation(_hv, _end_sentinel->elms, sizeof(hive_entry_t) * (HIVE_BUCKET_SIZE + 2));
    
    _hv->bucket_reserve.cap = 16;
    _hv->bucket_reserve.count = 0;
    _hv->bucket_reserve.array = HIVE_ALLOC_N(hive_bucket_t*, _hv->bucket_reserve.cap);
}

// TODO hive_clear

hive_bucket_t *hive_allocate_buckets(HIVE_NAME *_hv, size_t _nb)
{
    typedef hive_next_entry_t _next_entries_array[HIVE_BUCKET_SIZE + 2];
    typedef hive_entry_t _elms_array[HIVE_BUCKET_SIZE + 2];
    
    hive_bucket_t *_new_buckets = HIVE_ALLOC_N(hive_bucket_t, _nb);
    hive_next_entry_t (*_new_nexts)[HIVE_BUCKET_SIZE + 2] = HIVE_ALLOC_N(_next_entries_array, _nb);
    hive_next_entry_t (*_new_prevs)[HIVE_BUCKET_SIZE + 2] = HIVE_ALLOC_N(_next_entries_array, _nb);
    HIVE_TYPE (*_new_elms)[HIVE_BUCKET_SIZE + 2] = HIVE_ALLOC_N(_elms_array, _nb);
    
    hive_push_allocation(_hv, _new_buckets, sizeof(hive_bucket_t) * _nb);
    hive_push_allocation(_hv, _new_nexts, sizeof(hive_next_entry_t[HIVE_BUCKET_SIZE + 2]) * _nb);
    hive_push_allocation(_hv, _new_prevs, sizeof(hive_next_entry_t[HIVE_BUCKET_SIZE + 2]) * _nb);
    hive_push_allocation(_hv, _new_elms, sizeof(hive_bucket_t) * _nb);
    
    for(size_t _i = 0 ; _i < _nb ; _i++)
    {
        _new_buckets[_i].next_entries = _new_nexts[_i];
        _new_buckets[_i].prev_entries = _new_prevs[_i];
        _new_buckets[_i].elms = _new_elms[_i];
    }
    
    hive_push_to_buckets_reserve(_hv, _new_buckets, _nb);
    
    return _new_buckets;
}

HIVE_NAME hive_clone(const HIVE_NAME * _hv)
{
    HIVE_NAME _ret;
    hive_init(&_ret);
    _ret.count = _hv->count;
    _ret.bucket_count = _hv->bucket_count;
    
    if(_hv->not_full_buckets.count > _ret.not_full_buckets.cap)
    {
        _ret.not_full_buckets.array = HIVE_REALLOC_N(_ret.not_full_buckets.array, _ret.not_full_buckets.cap, _hv->not_full_buckets.count);
        _ret.not_full_buckets.cap = _hv->not_full_buckets.count;
    }
    _ret.not_full_buckets.count = _hv->not_full_buckets.count;
    
    hive_bucket_t *_new_buckets = hive_allocate_buckets(&_ret, _hv->bucket_count);// HIVE_ALLOC_N(hive_bucket_t, _hv->bucket_count);
//     hive_next_entry_t (*_new_nexts)[HIVE_BUCKET_SIZE + 2] = HIVE_ALLOC_N(hive_next_entry_t[HIVE_BUCKET_SIZE + 2], _hv->bucket_count);
//     hive_next_entry_t (*_new_prevs)[HIVE_BUCKET_SIZE + 2] = HIVE_ALLOC_N(hive_next_entry_t[HIVE_BUCKET_SIZE + 2], _hv->bucket_count);
//     
//     hive_push_allocation(&_ret, _new_buckets, sizeof(hive_bucket_t) * _hv->bucket_count);
//     hive_push_allocation(&_ret, _new_nexts, sizeof(hive_next_entry_t[HIVE_BUCKET_SIZE + 2]) * _hv->bucket_count);
//     hive_push_allocation(&_ret, _new_prevs, sizeof(hive_next_entry_t[HIVE_BUCKET_SIZE + 2]) * _hv->bucket_count);
    
    // hive_push_to_buckets_reserve(&_ret, _new_buckets, _hv->bucket_count);
    if(_hv->bucket_count > 0)
    {
        hive_bucket_t *_src_bucket = _hv->buckets;
        
        hive_bucket_t **_dst_bucket = &_ret.buckets;
        hive_bucket_t *_dst_prev = NULL;
        
        *_dst_bucket = _ret.bucket_reserve.array[--_ret.bucket_reserve.count];
        
        (*_dst_bucket)->count = _src_bucket->count;
        (*_dst_bucket)->not_full_idx = _src_bucket->not_full_idx;
        
        memcpy((*_dst_bucket)->next_entries, _src_bucket->next_entries, sizeof(hive_next_entry_t) * (HIVE_BUCKET_SIZE + 2));
        memcpy((*_dst_bucket)->prev_entries, _src_bucket->prev_entries, sizeof(hive_next_entry_t) * (HIVE_BUCKET_SIZE + 2));
        memcpy((*_dst_bucket)->elms,         _src_bucket->elms,         sizeof(hive_entry_t)      * (HIVE_BUCKET_SIZE + 2));
        
        (*_dst_bucket)->prev = NULL;
        
        if((*_dst_bucket)->not_full_idx != UINT16_MAX)
        {
            _ret.not_full_buckets.array[(*_dst_bucket)->not_full_idx] = *_dst_bucket;
        }
        
        memcpy((*_dst_bucket)->empty_bitset, _src_bucket->empty_bitset, sizeof(_src_bucket->empty_bitset));
        
        (*_dst_bucket)->next = NULL;
        _dst_prev = (*_dst_bucket);
        _dst_bucket = &(*_dst_bucket)->next;
        _src_bucket = _src_bucket->next;
        
        while(_src_bucket != _hv->end_sentinel)
        {
            *_dst_bucket = _ret.bucket_reserve.array[--_ret.bucket_reserve.count];
            
            (*_dst_bucket)->count = _src_bucket->count;
            (*_dst_bucket)->not_full_idx = _src_bucket->not_full_idx;
            
            memcpy((*_dst_bucket)->next_entries, _src_bucket->next_entries, sizeof(hive_next_entry_t) * (HIVE_BUCKET_SIZE + 2));
            memcpy((*_dst_bucket)->prev_entries, _src_bucket->prev_entries, sizeof(hive_next_entry_t) * (HIVE_BUCKET_SIZE + 2));
            memcpy((*_dst_bucket)->elms,         _src_bucket->elms,         sizeof(hive_entry_t)      * (HIVE_BUCKET_SIZE + 2));
            
            (*_dst_bucket)->next = NULL;
            (*_dst_bucket)->prev = _dst_prev;
            if((*_dst_bucket)->not_full_idx != UINT16_MAX)
            {
                _ret.not_full_buckets.array[(*_dst_bucket)->not_full_idx] = *_dst_bucket;
            }
            
            memcpy((*_dst_bucket)->empty_bitset, _src_bucket->empty_bitset, sizeof(_src_bucket->empty_bitset));
            
            _dst_prev->next = *_dst_bucket;
            _dst_prev = (*_dst_bucket);
            _dst_bucket = &(*_dst_bucket)->next;
            _src_bucket = _src_bucket->next;
        }
        _ret.tail = _dst_prev;
        _ret.tail->next = _ret.end_sentinel;
    }
    
    return _ret;
}

hive_iter hive_put(HIVE_NAME *_hv, HIVE_TYPE _new_elm)
{
    hive_iter _it = hive_put_uninit(_hv);
    *_it.ptr = _new_elm;
    return _it;
}

void hive_push_allocation(HIVE_NAME *_hv, void *_a, size_t _size)
{
    if(_hv->allocations.cap <= _hv->allocations.count)
    {
        size_t _new_cap = _hv->allocations.cap * 2;
        _hv->allocations.array = HIVE_REALLOC_N(_hv->allocations.array, _hv->allocations.cap, _new_cap);
        _hv->allocations.cap = _new_cap;
    }
    _hv->allocations.array[_hv->allocations.count].ptr = _a;
    _hv->allocations.array[_hv->allocations.count].size = _size;
    
    _hv->allocations.count += 1;
}

void hive_push_to_buckets_reserve(HIVE_NAME *_hv, hive_bucket_t *_new_buckets, size_t _nb_buckets)
{
    if(_hv->bucket_reserve.cap <= _hv->bucket_reserve.count + _nb_buckets)
    {
        size_t _new_cap = (_hv->bucket_reserve.count + _nb_buckets) * 2;
        _hv->bucket_reserve.array = HIVE_REALLOC_N(_hv->bucket_reserve.array, _hv->bucket_reserve.cap, _new_cap);
        _hv->bucket_reserve.cap = _new_cap;
    }
    
    for(size_t i = 0 ; i < _nb_buckets ; i++)
    {
        _hv->bucket_reserve.array[i + _hv->bucket_reserve.count] = &_new_buckets[i];
    }
    
    _hv->bucket_reserve.count += _nb_buckets;
}

void hive_push_used_bucket_to_reserve(HIVE_NAME *_hv, hive_bucket_t *_bucket)
{
    if(_hv->bucket_reserve.cap <= _hv->bucket_reserve.count + 1)
    {
        size_t _new_cap = (_hv->bucket_reserve.count + 1) * 2;
        _hv->bucket_reserve.array = HIVE_REALLOC_N(_hv->bucket_reserve.array, _hv->bucket_reserve.cap, _new_cap);
        _hv->bucket_reserve.cap = _new_cap;
    }
    
    _hv->bucket_reserve.array[_hv->bucket_reserve.count] = _bucket;
    
    _hv->bucket_reserve.count += 1;
}

hive_iter hive_put_uninit(HIVE_NAME *_hv)
{
    HIVE_TYPE *_elm_added = NULL;
    
    if(_hv->not_full_buckets.count > 0)
    {
        hive_bucket_t *_bucket = _hv->not_full_buckets.array[_hv->not_full_buckets.count - 1];
        _hv->count += 1;
        HIVE_TYPE *_elm = hive_bucket_reserve_slot(_hv, _bucket);
        return hive_get_iter_from_index(_bucket, _elm - &_bucket->elms[0]);
    }
    if(_hv->bucket_reserve.count == 0)
    {
        size_t _nb_buckets_to_alloc = _hv->bucket_count + (_hv->bucket_count == 0); // double the number of current buckets
        hive_bucket_t *_new_buckets = hive_allocate_buckets(_hv, _nb_buckets_to_alloc); //HIVE_ALLOC_N(hive_bucket_t, _nb_buckets_to_alloc);
        // hive_next_entry_t (*_new_nexts)[HIVE_BUCKET_SIZE + 2] = HIVE_ALLOC_N(hive_next_entry_t[HIVE_BUCKET_SIZE + 2], _nb_buckets_to_alloc);
        // hive_next_entry_t (*_new_prevs)[HIVE_BUCKET_SIZE + 2] = HIVE_ALLOC_N(hive_next_entry_t[HIVE_BUCKET_SIZE + 2], _nb_buckets_to_alloc);
        // hive_push_allocation(_hv, _new_buckets, sizeof(hive_bucket_t) * _nb_buckets_to_alloc);
        // hive_push_allocation(_hv, _new_nexts, sizeof(hive_next_entry_t[HIVE_BUCKET_SIZE + 2]) * _nb_buckets_to_alloc);
        // hive_push_allocation(_hv, _new_prevs, sizeof(hive_next_entry_t[HIVE_BUCKET_SIZE + 2]) * _nb_buckets_to_alloc);
        // hive_push_to_buckets_reserve(_hv, _new_buckets, _new_nexts, _new_prevs, _nb_buckets_to_alloc);
    }
    
    hive_bucket_t *_new_bucket = _hv->bucket_reserve.array[--_hv->bucket_reserve.count];
    hive_bucket_init(_new_bucket);
    hive_push_not_full_bucket(_hv, _new_bucket);
    
    if(_hv->count > 0)
    {
        _hv->tail->next = _new_bucket;
        _new_bucket->prev = _hv->tail;
        _new_bucket->next = _hv->end_sentinel;
        _hv->tail = _new_bucket;
    }
    else
    {
        _hv->buckets = _new_bucket;
        _hv->tail = _new_bucket;
        _new_bucket->next = _hv->end_sentinel;
    }
    
    _hv->end_sentinel->prev = _new_bucket;
    _hv->bucket_count += 1;
    _elm_added = hive_bucket_reserve_slot(_hv, _new_bucket);
    _hv->count += 1;
    return hive_get_iter_from_index(_new_bucket, _elm_added - &_new_bucket->elms[0]);
}

void hive_put_all(HIVE_NAME *_hv, const HIVE_TYPE *_elms, size_t _nelms)
{
    typedef hive_next_entry_t _next_entries_array[HIVE_BUCKET_SIZE + 2];
    typedef hive_entry_t _elms_array[HIVE_BUCKET_SIZE + 2];
    
    size_t _buckets_to_fill = _nelms / HIVE_BUCKET_SIZE;
    ptrdiff_t _remaining = _nelms - (_buckets_to_fill * HIVE_BUCKET_SIZE);
    
    hive_bucket_t *_first = NULL;
    bool _first_set = false;
    hive_bucket_t *_bucket = NULL;
    hive_bucket_t *_prev = NULL;
    
    // TODO rework this. make one allocation for all buckets needed.
    // and push that allocation
    size_t _nb_allocated = _buckets_to_fill + (_remaining != 0);
    hive_bucket_t *_new_buckets = HIVE_ALLOC_N(hive_bucket_t, _nb_allocated);
    hive_next_entry_t (*_new_nexts)[HIVE_BUCKET_SIZE + 2] = HIVE_ALLOC_N(_next_entries_array, _nb_allocated);
    hive_next_entry_t (*_new_prevs)[HIVE_BUCKET_SIZE + 2] = HIVE_ALLOC_N(_next_entries_array, _nb_allocated);
    typeof(HIVE_TYPE) (*_new_elms)[HIVE_BUCKET_SIZE + 2] = HIVE_ALLOC_N(_elms_array, _nb_allocated);
    for(size_t i = 0 ; i < _nb_allocated ; i++)
    {
        _new_buckets[i].next_entries = _new_nexts[i];
        _new_buckets[i].prev_entries = _new_prevs[i];
        _new_buckets[i].elms = _new_elms[i];
    }
    
    hive_push_allocation(_hv, _new_buckets, sizeof(hive_bucket_t) * _nb_allocated);
    hive_push_allocation(_hv, _new_nexts, sizeof(hive_next_entry_t[HIVE_BUCKET_SIZE + 2]) * _nb_allocated);
    hive_push_allocation(_hv, _new_prevs, sizeof(hive_next_entry_t[HIVE_BUCKET_SIZE + 2]) * _nb_allocated);
    hive_push_allocation(_hv, _new_elms, sizeof(hive_entry_t[HIVE_BUCKET_SIZE + 2]) * _nb_allocated);
    
    for(size_t _i = 0 ; _i < _buckets_to_fill ; _i++)
    {
        _bucket = &_new_buckets[_i];
        if(!_first_set)
        {
            _first = _bucket;
            _first_set = true;
        }
        hive_bucket_init(_bucket);
        memcpy(_bucket->elms + 1, _elms + (_i * HIVE_BUCKET_SIZE), HIVE_BUCKET_SIZE * sizeof(hive_entry_t));
        for(int _j = 0 ; _j <= HIVE_END_SENTINEL_INDEX ; _j++)
        {
            _bucket->next_entries[_j].next_elm_index = _j;
            _bucket->prev_entries[_j].next_elm_index = _j;
        }
        
        memset(_bucket->empty_bitset, 0, sizeof(_bucket->empty_bitset));
        _bucket->count = HIVE_BUCKET_SIZE;
        _hv->bucket_count += 1;
        _hv->count += HIVE_BUCKET_SIZE;
        
        _bucket->prev = _prev;
        if(_prev)
        {
            _prev->next = _bucket;
        }
        _prev = _bucket;
    }
    
    if(_remaining > 0)
    {
        hive_bucket_t *_remaining_bucket = &_new_buckets[_buckets_to_fill];
        hive_bucket_init(_remaining_bucket);
        memcpy(_remaining_bucket->elms + 1, _elms + (_buckets_to_fill * HIVE_BUCKET_SIZE), _remaining * sizeof(HIVE_TYPE));
        for(uint8_t _j = 0 ; _j < _remaining + 1; _j++)
        {
            _remaining_bucket->next_entries[_j].next_elm_index = _j;
            _remaining_bucket->prev_entries[_j].next_elm_index = _j;
        }
        _remaining_bucket->prev_entries[_remaining + 1].next_elm_index = _remaining;
        _remaining_bucket->prev_entries[HIVE_END_SENTINEL_INDEX - 1].next_elm_index = _remaining;
        
        _remaining_bucket->next_entries[HIVE_END_SENTINEL_INDEX].next_elm_index = HIVE_END_SENTINEL_INDEX;
        _remaining_bucket->prev_entries[HIVE_END_SENTINEL_INDEX].next_elm_index = HIVE_END_SENTINEL_INDEX;
        
        for(int i = 1 ; i < _remaining + 1 ; i++)
        {
            _remaining_bucket->empty_bitset[i / 64] &= ~((uint64_t)1 << (i % 64));
        }
        
        _remaining_bucket->count = _remaining;
        
        hive_push_not_full_bucket(_hv, _remaining_bucket);
        
        if(_bucket)
        {
            _bucket->next = _remaining_bucket;
        }
        
        _remaining_bucket->prev = _bucket;
        _bucket = _remaining_bucket;
        
        if(!_first)
        {
            _first = _remaining_bucket;
        }
        
        _hv->count += _remaining;
        _hv->bucket_count += 1;
    }
    
    if(_hv->buckets != _hv->end_sentinel && _first != NULL)
    {
        _hv->tail->next = _first;
        _hv->tail = _bucket;
        _hv->tail->next = _hv->end_sentinel;
        _hv->end_sentinel->prev = _hv->tail;
    }
    else
    {
        _hv->buckets = _first;
        _hv->tail = _bucket;
        _hv->tail->next = _hv->end_sentinel;
        _hv->end_sentinel->prev = _hv->tail;
    }
}

hive_iter hive_del_helper(HIVE_NAME *_hv, hive_bucket_t *_prev_bucket, hive_bucket_t *_bucket, uint8_t _index)
{
    hive_iter _ret;
    if(hive_bucket_del(_hv, _bucket, _index))
    {
        if(_prev_bucket != NULL)
        {
            _prev_bucket->next = _bucket->next;
        }
        else
        {
            _hv->buckets = _bucket->next;
        }
        if(_bucket->next != NULL)
        {
            _bucket->next->prev = _prev_bucket;
        }
        
        if(_bucket == _hv->tail)
        {
            if(_prev_bucket != NULL)
            {
                _hv->tail = _prev_bucket;
            }
            else
            {
                _hv->tail = _hv->end_sentinel;
                _hv->end_sentinel->prev = NULL;
            }
            
            _ret = hive_get_iter_from_index(_hv->end_sentinel, HIVE_END_SENTINEL_INDEX);
        }
        else
        {
            uint8_t _first_elm = hive_bucket_first_elm(_bucket->next);
            _ret = hive_get_iter_from_index(_bucket->next, _first_elm);
        }
        
        hive_push_used_bucket_to_reserve(_hv, _bucket);
        _hv->bucket_count -= 1;
    }
    else
    {
        uint8_t _next_elm = _bucket->next_entries[_index + 1].next_elm_index;
        
        if(_next_elm == HIVE_END_SENTINEL_INDEX)
        {
            hive_bucket_t *_next_bucket = _bucket->next;
            uint8_t _first_elm_in_next_bucket = hive_bucket_first_elm(_next_bucket);
            _ret = hive_get_iter_from_index(_next_bucket, _first_elm_in_next_bucket);
        }
        else
        {
            _ret = hive_get_iter_from_index(_bucket, _next_elm);
        }
    }
    _hv->count -= 1;
    return _ret;
}


hive_iter hive_del(HIVE_NAME *_hv, HIVE_TYPE *_elm)
{
    hive_iter _ret = {0};
    for(hive_bucket_t *_bucket = _hv->buckets ; _bucket != _hv->end_sentinel ; _bucket = _bucket->next)
    {
        if(hive_bucket_is_elm_within(_bucket, _elm))
        {
            hive_entry_t *as_entry = (hive_entry_t*) _elm;
            uint8_t index = as_entry - _bucket->elms;
            _ret = hive_del_helper(_hv, _bucket->prev, _bucket, index);
            break;
        }
    }
    return _ret;
}

void hive_foreach_updater(uint8_t *_index, hive_bucket_t **_bucket)
{
    ++(*_index);
    (*_index) = (*_bucket)->next_entries[*_index].next_elm_index;
    if(*_index == HIVE_END_SENTINEL_INDEX)
    {
        (*_bucket) = (*_bucket)->next;
        (*_index) = hive_bucket_first_elm((*_bucket));;
    }
}

void hive_foreach(HIVE_NAME *_hv, void(*_f)(HIVE_TYPE*,void*), void *_arg)
{
    hive_bucket_t *_bucket = _hv->buckets;
    uint8_t _index = hive_bucket_first_elm(_bucket);
    for( ; _bucket != _hv->end_sentinel ; hive_foreach_updater(&_index, &_bucket))
    {
        _f(&_bucket->elms[_index], _arg);
    }
}

void hive_deinit(HIVE_NAME *_hv)
{
    for(size_t i = 0 ; i < _hv->allocations.count ; i++)
    {
        HIVE_FREE(HIVE_ALLOC_CTX, _hv->allocations.array[i].ptr, _hv->allocations.array[i].size);
    }
    HIVE_FREE_N(_hv->allocations.array, _hv->allocations.cap);
    HIVE_FREE_N(_hv->bucket_reserve.array, _hv->bucket_reserve.cap);
    HIVE_FREE_N(_hv->not_full_buckets.array, _hv->not_full_buckets.cap);
    
    *_hv = (HIVE_NAME){0};
}

void hive_bucket_init(hive_bucket_t *_bucket)
{
    _bucket->next = NULL;
    _bucket->prev = NULL;
    _bucket->not_full_idx = UINT16_MAX;
    _bucket->count = 0;
    
    for(int _i = 0 ; _i <= HIVE_END_SENTINEL_INDEX ; _i++)
    {
        _bucket->next_entries[_i].next_elm_index = HIVE_END_SENTINEL_INDEX;
    }
    
    for(int _i = 0 ; _i <= HIVE_END_SENTINEL_INDEX ; _i++)
    {
        _bucket->prev_entries[_i].next_elm_index = 0;
    }
    
    // first and last bits 0, bits in middle 1
    // for begin and end sentinels
    _bucket->empty_bitset[0] = UINT64_MAX - 1;
    for(int i = 1 ; i <= 2 ; i++)
        _bucket->empty_bitset[i] = UINT64_MAX;
    _bucket->empty_bitset[3] = UINT64_MAX & ~((uint64_t)1 << 63);
}

void hive_push_not_full_bucket(HIVE_NAME *_hv, hive_bucket_t *_bucket)
{
    if(_hv->not_full_buckets.cap <= _hv->not_full_buckets.count)
    {
        uint16_t _new_cap = _hv->not_full_buckets.cap * 2;
        _hv->not_full_buckets.array = HIVE_REALLOC_N(_hv->not_full_buckets.array, _hv->not_full_buckets.cap, _new_cap);
        _hv->not_full_buckets.cap = _new_cap;
    }
    
    _hv->not_full_buckets.array[_hv->not_full_buckets.count] = _bucket;
    _bucket->not_full_idx = _hv->not_full_buckets.count;
    
    _hv->not_full_buckets.count += 1;
}

HIVE_TYPE *hive_bucket_reserve_slot(HIVE_NAME *_hv, hive_bucket_t *_bucket)
{
    assert(_bucket->count < HIVE_BUCKET_SIZE);
    
    uint8_t _empty_index = hive_bitset_set_first_empty(&_bucket->empty_bitset);
    assert(_bucket->next_entries[_empty_index].next_elm_index != _empty_index);
    
    // if(_bucket->next_entries[_empty_index + 1].next_elm_index != _empty_index + 1)
    {
        uint8_t _next_elm_idx = _bucket->next_entries[_empty_index].next_elm_index;
        
        _bucket->next_entries[_empty_index  + 1].next_elm_index = _next_elm_idx;
        _bucket->prev_entries[_next_elm_idx - 1].next_elm_index = _empty_index;
    }
    
    _bucket->next_entries[_empty_index].next_elm_index = _empty_index;
    _bucket->prev_entries[_empty_index].next_elm_index = _empty_index;
    _bucket->count += 1;
    
    // TODO try not having `first_elm_idx` and instead use the bitset. might be faster, needs benchmarking.
    
    if(_bucket->count == HIVE_BUCKET_SIZE)
    {
        _bucket->not_full_idx = UINT16_MAX;
        _hv->not_full_buckets.count -= 1;
    }
    
    return &_bucket->elms[_empty_index];
}

bool hive_bucket_del(HIVE_NAME *_hv, hive_bucket_t *_bucket, uint8_t _index)
{
    (void)_hv;
    assert(_bucket->next_entries[_index].next_elm_index == _index);
    assert(_bucket->count != 0);
    
    // TODO if prev and next elms are holes, make the prev actual elm point to the end of the new big hole
    // if prev and next are acutal elms, just change this elm's next_entry
    // if prev is actual elm but next is hole, set this elm's next_entry to skip the entire hole
    _bucket->empty_bitset[_index / 64] |= ((uint64_t)1 << (_index % 64));
    
    const uint8_t _next_elm = _bucket->next_entries[_index + 1].next_elm_index;
    const uint8_t _prev_elm = _bucket->prev_entries[_index - 1].next_elm_index;
    
    _bucket->next_entries[_prev_elm + 1].next_elm_index = _bucket->next_entries[_index + 1].next_elm_index;
    _bucket->prev_entries[_next_elm - 1].next_elm_index = _bucket->prev_entries[_index - 1].next_elm_index;
    
    // _bucket->next_entries[_index].next_elm_index = _bucket->next_entries[_index + 1].next_elm_index;
    // _bucket->prev_entries[_index].next_elm_index = _bucket->prev_entries[_index - 1].next_elm_index;
    
    if(_bucket->count == 1)
    {
        _hv->not_full_buckets.array[_bucket->not_full_idx] = _hv->not_full_buckets.array[_hv->not_full_buckets.count-1];
        _hv->not_full_buckets.array[_bucket->not_full_idx]->not_full_idx = _bucket->not_full_idx;
        _hv->not_full_buckets.count -= 1;
        
        _bucket->count -= 1;
        return true;
    }
    else if(_bucket->count == HIVE_BUCKET_SIZE)
    {
        hive_push_not_full_bucket(_hv, _bucket);
    }
    
    _bucket->count -= 1;
    
    return false;
}

uint8_t hive_bucket_first_elm(const hive_bucket_t *_bucket)
{
    return hive_bitset_first_elm(&_bucket->empty_bitset);
}

bool hive_bucket_is_elm_within(const hive_bucket_t *_bucket, const HIVE_TYPE *_elm)
{
    uintptr_t
    _ibegin = (uintptr_t) (_bucket->elms + 1),
    _iend   = (uintptr_t) (_bucket->elms + HIVE_END_SENTINEL_INDEX),
    _ielm   = (uintptr_t) (_elm);
    return _ielm >= _ibegin && _ielm < _iend;
}

hive_iter hive_begin(const HIVE_NAME *_hv)
{
    return hive_get_iter_from_index(_hv->buckets, hive_bucket_first_elm(_hv->buckets));
}

hive_iter hive_end(const HIVE_NAME *_hv)
{
    return hive_get_iter_from_index(_hv->end_sentinel, HIVE_END_SENTINEL_INDEX);
}

hive_iter hive_get_iter_from_index(hive_bucket_t *_bucket, uint8_t _index)
{
    hive_iter _ret;
    _ret.bucket = _bucket;
    _ret.ptr = &_bucket->elms[_index];
    _ret.next_entry = &_ret.bucket->next_entries[_index + 1];
    return _ret;
}

bool hive_iter_eq(hive_iter _a, hive_iter _b)
{
    return (_a.ptr == _b.ptr);
}

hive_iter hive_iter_next(hive_iter _it)
{
    uint8_t _index = _it.next_entry->next_elm_index;
    
    if (HIVE_UNLIKELY(_index == HIVE_END_SENTINEL_INDEX))
    {
        _it.bucket = _it.bucket->next;
        _index = hive_bucket_first_elm(_it.bucket);
    }
    _it.ptr = &_it.bucket->elms[_index];
    _it.next_entry = &_it.bucket->next_entries[_index] + 1;
    
    return _it;
}

hive_iter hive_iter_del(HIVE_NAME *_hv, hive_iter _it)
{
    uint8_t _index = _it.ptr - _it.bucket->elms;
    
    return hive_del_helper(_hv, _it.bucket->prev, _it.bucket, _index);
}

hive_iter hive_ptr_to_iter(HIVE_NAME *_hive, hive_entry_t *_ptr)
{
    hive_bucket_t *_bucket = _hive->buckets;
    while(_bucket != _hive->end_sentinel)
    {
        if(hive_bucket_is_elm_within(_bucket, _ptr))
        {
            uint8_t _index = _ptr - _bucket->elms;
            return hive_get_iter_from_index(_bucket, _index);
        }
        
        _bucket = _bucket->next;
    }
    
    return (hive_iter){0};
}

hive_handle hive_iter_to_handle(hive_iter _it)
{
    hive_handle _ret;
    _ret.bucket = _it.bucket;
    _ret.ptr = _it.ptr;
    return _ret;
}

hive_iter hive_handle_to_iter(hive_handle _handle)
{
    uint8_t _index = _handle.ptr - _handle.bucket->elms;
    return hive_get_iter_from_index(_handle.bucket, _index);
}

hive_iter hive_handle_del(HIVE_NAME *_hv, hive_handle _handle)
{
    uint8_t _index = _handle.ptr - _handle.bucket->elms;
    
    return hive_del_helper(_hv, _handle.bucket->prev, _handle.bucket, _index);
}

hive_handle hive_ptr_to_handle(HIVE_NAME *_hive, hive_entry_t *_ptr)
{
    hive_bucket_t *_bucket = _hive->buckets;
    while(_bucket != _hive->end_sentinel)
    {
        if(hive_bucket_is_elm_within(_bucket, _ptr))
        {
            hive_handle _ret;
            _ret.ptr = _ptr;
            _ret.bucket = _bucket;
            return _ret;
        }
        
        _bucket = _bucket->next;
    }
    
    return (hive_handle){0};
}

#ifdef HIVE_TEST

hive_iter hive_checked_put(HIVE_NAME *_hive, HIVE_TYPE _elm)
{
    if(1) //_hive->not_full_buckets.count == 0)
        return hive_put(_hive, _elm);
    
//     hive_bucket_t *_bucket = _hive->not_full_buckets.array[_hive->not_full_buckets.count - 1];
//     
//     uint16_t _old_not_full_count = _hive->not_full_buckets.count;
//     uint8_t _old_count = _bucket->count;
//     bool _will_fill = _bucket->count == (HIVE_BUCKET_SIZE - 1);
//     
//     uint8_t _index = hive_bucket_first_empty(_bucket);
//     
//     hive_next_entry_t _old_nexts[HIVE_BUCKET_SIZE + 2];
//     memcpy(_old_nexts, _bucket->next_entries, sizeof(_old_nexts));
//     
//     hive_next_entry_t _old_prevs[HIVE_BUCKET_SIZE + 2];
//     memcpy(_old_prevs, _bucket->prev_entries, sizeof(_old_prevs));
//     
//     HIVE_TYPE *_old_elms = HIVE_ALLOC_N(HIVE_TYPE, HIVE_BUCKET_SIZE + 2);
//     memcpy(_old_elms, _bucket->elms, sizeof(_bucket->elms));
//     
//     uint64_t _old_bitset[4];
//     memcpy(_old_bitset, _bucket->empty_bitset, sizeof(_old_bitset));
//     
//     hive_iter _ret = hive_put(_hive, _elm);
//     
//     assert(_old_count == _bucket->count - 1);
//     if(_will_fill)
//     {
//         assert(_old_not_full_count == _hive->not_full_buckets.count + 1);
//         assert(_bucket->not_full_idx == UINT16_MAX);
//     }
//     
//     for(uint8_t i = 0 ; i < _index ; i++)
//     {
//         assert(_old_nexts[i].next_elm_index == _bucket->next_entries[i].next_elm_index);
//         assert(i == 0 || _bucket->next_entries[i].next_elm_index == i);
//     }
//     assert(_old_nexts[_index].next_elm_index != _bucket->next_entries[_index].next_elm_index);
//     assert(_bucket->next_entries[_index].next_elm_index == _index);
//     for(int i = _index + 2 ; i < HIVE_END_SENTINEL_INDEX ; i++)
//     {
//         assert(_old_nexts[i].next_elm_index == _bucket->next_entries[i].next_elm_index);
//     }
//     
//     for(uint8_t i = 0 ; i < _index ; i++)
//     {
//         assert(_old_prevs[i].next_elm_index == _bucket->prev_entries[i].next_elm_index);
//         assert(i == 0 || _bucket->prev_entries[i].next_elm_index == i);
//     }
//     // assert(_old_prevs[_index].next_elm_index != _bucket->prev_entries[_index].next_elm_index);
//     assert(_bucket->prev_entries[_index].next_elm_index == _index);
//     // uint8_t _hole_after_index = _index + 1;
// //     for( ; _hole_after_index < HIVE_END_SENTINEL_INDEX ; _hole_after_index++)
// //     {
// //         if(_old_prevs[_hole_after_index].next_elm_index == _hole_after_index)
// //             break;
// //     }
// //     
// //     if(_hole_after_index != HIVE_END_SENTINEL_INDEX)
// //     {
// //         assert(_bucket->prev_entries[_hole_after_index - 1].next_elm_index == _index);
// //     }
// //     
// //     for(uint8_t i = _hole_after_index ; i < HIVE_END_SENTINEL_INDEX ; i++)
// //     {
// //         assert(_bucket->prev_entries[i].next_elm_index == _old_prevs[i].next_elm_index);
// //     }
//     
//     HIVE_FREE_N(_old_elms, HIVE_BUCKET_SIZE + 2);
//     return _ret;
}

hive_iter hive_checked_del(HIVE_NAME *_hive, HIVE_TYPE *_elm)
{
    hive_bucket_t *_bucket = _hive->buckets;
    while(_bucket != _hive->end_sentinel)
    {
        if(hive_bucket_is_elm_within(_bucket, _elm))
        {
            uint8_t _index = _elm - _bucket->elms;
            hive_iter _it = hive_get_iter_from_index(_bucket, _index);
            return hive_checked_iter_del(_hive, _it);
        }
        _bucket = _bucket->next;
    }
    assert(0);
    return (hive_iter){0};
}

hive_iter hive_checked_iter_del(HIVE_NAME *_hive, hive_iter _it)
{
    hive_bucket_t *_bucket = _it.bucket;
    
    if(_bucket->count == 1)
    {
        return hive_iter_del(_hive, _it);
    }
    
    uint16_t _old_not_full_count = _hive->not_full_buckets.count;
    uint8_t _old_count = _bucket->count;
    bool _will_unfill = (_bucket->count == HIVE_BUCKET_SIZE);
    
    uint8_t _index = _it.ptr - _bucket->elms;
    
    hive_next_entry_t _old_nexts[HIVE_BUCKET_SIZE + 2];
    memcpy(_old_nexts, _bucket->next_entries, sizeof(_old_nexts));
    
    hive_next_entry_t _old_prevs[HIVE_BUCKET_SIZE + 2];
    memcpy(_old_prevs, _bucket->prev_entries, sizeof(_old_prevs));
    
    HIVE_TYPE *_old_elms = HIVE_ALLOC_N(HIVE_TYPE, HIVE_BUCKET_SIZE + 2);
    memcpy(_old_elms, _bucket->elms, sizeof(_bucket->elms));
    
    uint64_t _old_bitset[4];
    memcpy(_old_bitset, _bucket->empty_bitset, sizeof(_old_bitset));
    
    hive_iter _ret = hive_iter_del(_hive, _it);
    
    assert(_bucket->count == _old_count - 1);
    
    if(_will_unfill)
    {
        assert(_bucket->not_full_idx != UINT16_MAX);
        assert(_old_not_full_count == _hive->not_full_buckets.count - 1);
    }
    
    // assert(_bucket->next_entries[_index].next_elm_index != _index);
    // assert(_bucket->prev_entries[_index].next_elm_index != _index);
    
    HIVE_FREE_N(_old_elms, HIVE_BUCKET_SIZE + 2);
    
    return _ret;
}

#endif // HIVE_TEST

#endif // HIVE_IMPL

#undef HIVE_TYPE
#undef HIVE_NAME
#undef HIVE_IMPL
#undef HIVE_ALLOC
#undef HIVE_FREE
#undef HIVE_REALLOC
#undef HIVE_ALLOC_CTX
#undef HIVE_ALLOC_N
#undef HIVE_REALLOC_N
#undef HIVE_FREE_N
#undef HIVE_LIKELY
#undef HIVE_UNLIKELY

#undef HIVE_CAT_
#undef HIVE_CAT

#undef hive_iter
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
#undef hive_bucket_reserve_slot
#undef hive_bucket_del
#undef hive_bucket_is_elm_within
#undef hive_bucket_first_elm
#undef hive_iter_next
#undef hive_iter_del
#undef hive_iter_eq
#undef hive_iter_is_end
#undef hive_iter_to
#undef hive_handle
#undef hive_handle_del
#undef hive_iter_to_handle
#undef hive_ptr_to_iter
#undef hive_ptr_to_handle

#undef hive_checked_put
#undef hive_checked_del
#undef hive_checked_iter_del

#undef HIVE_DECLARED
