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
#define hive_next_entry_t   HIVE_CAT(HIVE_NAME, _next_entry_t)
#define hive_init           HIVE_CAT(HIVE_NAME, _init)
#define hive_clone          HIVE_CAT(HIVE_NAME, _clone)
#define hive_put            HIVE_CAT(HIVE_NAME, _put)
#define hive_put_all        HIVE_CAT(HIVE_NAME, _put_all)
#define hive_del            HIVE_CAT(HIVE_NAME, _del)
#define hive_foreach        HIVE_CAT(HIVE_NAME, _foreach)
#define hive_begin          HIVE_CAT(HIVE_NAME, _begin)
#define hive_end            HIVE_CAT(HIVE_NAME, _end)
#define hive_deinit         HIVE_CAT(HIVE_NAME, _deinit)

#define hive_iter_next      HIVE_CAT(HIVE_NAME, _iter_next)
#define hive_iter_elm       HIVE_CAT(HIVE_NAME, _iter_elm)
#define hive_iter_del       HIVE_CAT(HIVE_NAME, _iter_del)
#define hive_iter_eq        HIVE_CAT(HIVE_NAME, _iter_eq)

#define hive_handle_elm     HIVE_CAT(HIVE_NAME, _handle_elm)
#define hive_handle_del     HIVE_CAT(HIVE_NAME, _handle_del)
#define hive_iter_to_handle HIVE_CAT(HIVE_NAME, _iter_to_handle)

#define HIVE_ARR_LEN(arr) \
sizeof(arr) / sizeof(arr[0])

#define hive_entry_t typeof(HIVE_TYPE)

#if !defined(HIVE_DECLARED)

#define HIVE__FOREACH_NEXT(it) \
((it).next_entry->next_elm_index == HIVE_END_SENTINEL_INDEX ? \
(typeof((it))){ \
    .bucket = (it).bucket->next, \
    .next_entry = &(it).bucket->next->next_entries[(it).bucket->next->first_elm_idx] + 1, \
    .elm = &(it).bucket->next->elms[(it).bucket->next->first_elm_idx] \
} : \
(typeof((it))){ \
    .bucket = (it).bucket, \
    .next_entry = &(it).bucket->next_entries[(it).next_entry->next_elm_index] + 1, \
    .elm = &(it).bucket->elms[(it).next_entry->next_elm_index] \
} \
)

#define HIVE_FOR_EACH(name, from_it, to_it) \
for(typeof(from_it) name = (from_it) ; name.elm != (to_it).elm ; name = HIVE__FOREACH_NEXT(name))

typedef struct HIVE_NAME
{
    struct hive_bucket_t *buckets;
    struct hive_bucket_t *tail;
    struct hive_bucket_t *end_sentinel; // tail->next == end_sentinel
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
    hive_entry_t *elm;
} hive_iter;

typedef struct hive_handle
{
    struct hive_bucket_t *bucket;
    hive_entry_t *elm;
} hive_handle;

#endif // HIVE_DECLARED

#ifndef HIVE_COMMON_UTILS
#define HIVE_COMMON_UTILS

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

void hive_init(HIVE_NAME *_hv);
HIVE_NAME hive_clone(const HIVE_NAME *_hv);
hive_iter hive_put(HIVE_NAME *hv, hive_entry_t _new_elm);
void hive_put_all(HIVE_NAME *_hv, const hive_entry_t *_elms, size_t _nelms);
hive_iter hive_del(HIVE_NAME *_hv, hive_entry_t *_elm);
void hive_foreach(HIVE_NAME *_hv, void(*_f)(hive_entry_t*,void*), void *_arg);
void hive_deinit(HIVE_NAME *_hv);

hive_iter hive_begin(const HIVE_NAME *_hv);
hive_iter hive_end(const HIVE_NAME *_hv);
hive_iter hive_iter_next(hive_iter _it);
hive_entry_t *hive_iter_elm(hive_iter _it);
hive_iter hive_iter_del(HIVE_NAME *_hv, hive_iter _it);
bool hive_iter_eq(hive_iter _a, hive_iter _b);

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
    uint8_t first_elm_idx;
    uint8_t count;
    hive_entry_t elms[HIVE_BUCKET_SIZE + 2];
    hive_next_entry_t next_entries[HIVE_BUCKET_SIZE + 2];
    hive_next_entry_t prev_entries[HIVE_BUCKET_SIZE + 2];
} hive_bucket_t;

#define hive_push_not_full_bucket   HIVE_CAT(HIVE_NAME, _push_not_full_bucket)
#define hive_foreach_updater        HIVE_CAT(HIVE_NAME, _foreach_updater)

#define hive_bucket_init            HIVE_CAT(HIVE_NAME, _bucket_init)
#define hive_bucket_put             HIVE_CAT(HIVE_NAME, _bucket_put)
#define hive_bucket_del             HIVE_CAT(HIVE_NAME, _bucket_del)
#define hive_bucket_is_elm_within   HIVE_CAT(HIVE_NAME, _bucket_is_elm_within)
#define hive_bucket_first_elm       HIVE_CAT(HIVE_NAME, _bucket_first_elm)
#define hive_bucket_first_empty     HIVE_CAT(HIVE_NAME, _bucket_first_empty)
#define hive_bucket_set_first_empty HIVE_CAT(HIVE_NAME, _bucket_set_first_empty)
#define hive_del_helper             HIVE_CAT(HIVE_NAME, _del_helper)

#define hive_iter_to                HIVE_CAT(HIVE_NAME, _iter_to)

#ifdef HIVE_TEST
    #define hive_checked_put            HIVE_CAT(HIVE_NAME, _checked_put)
    #define hive_checked_del            HIVE_CAT(HIVE_NAME, _checked_del)
    #define hive_iter_checked_del       HIVE_CAT(HIVE_NAME, _iter_checked_del)

    hive_iter hive_checked_put(HIVE_NAME *_hive, HIVE_TYPE elm);
    hive_iter hive_checked_del(HIVE_NAME *_hive, HIVE_TYPE *elm);
    hive_iter hive_iter_checked_del(HIVE_NAME *_hive, hive_iter it);
#endif

void hive_foreach_updater(uint8_t *_index, hive_bucket_t **_bucket);
void hive_bucket_init(hive_bucket_t *_bucket);
void hive_push_not_full_bucket(HIVE_NAME *_hv, hive_bucket_t *_bucket);
HIVE_TYPE *hive_bucket_put(HIVE_NAME *_hv, hive_bucket_t *_bucket, HIVE_TYPE _new_elm);
bool hive_bucket_del(HIVE_NAME *_hv, hive_bucket_t *_bucket, uint8_t _index);
hive_iter hive_del_helper(HIVE_NAME *_hv, hive_bucket_t *_prev_bucket, hive_bucket_t *_bucket, uint8_t _index);
bool hive_bucket_is_elm_within(const hive_bucket_t *_bucket, const HIVE_TYPE *_elm);
uint8_t hive_bucket_first_elm(const hive_bucket_t *_bucket);
uint8_t hive_bucket_first_empty(const hive_bucket_t *_bucket);
uint8_t hive_bucket_set_first_empty(hive_bucket_t *_bucket);
hive_iter hive_iter_to(hive_bucket_t *_bucket, uint8_t _index);

void *hive_alloc_mem(void *_ctx, size_t _size, size_t _alignment);
void *hive_realloc_mem(void *_ctx, void *_ptr, size_t _old_size, size_t _new_size, size_t _alignment);
void hive_free_mem(void *_ctx, void *_ptr, size_t _size);

#define HIVE_ALLOC_N(type, count) \
(typeof(type)*) HIVE_ALLOC(HIVE_ALLOC_CTX, sizeof(type) * (count), alignof(type))

#define HIVE_REALLOC_N(ptr, old_count, new_count) \
(typeof(ptr)) HIVE_REALLOC(HIVE_ALLOC_CTX, (void*)(ptr), sizeof((ptr)[0]) * (old_count), sizeof((ptr)[0]) * (new_count), alignof(typeof((ptr)[0])))

#define HIVE_FREE_N(ptr, count) \
HIVE_FREE(HIVE_ALLOC_CTX, (void*)(ptr), sizeof(ptr[0]) * (count))

void hive_init(HIVE_NAME *_hv)
{
    hive_bucket_t *_end_sentinel = (hive_bucket_t*) HIVE_ALLOC(HIVE_ALLOC_CTX, sizeof(hive_bucket_t), alignof(hive_bucket_t));
    hive_bucket_init(_end_sentinel);
    _end_sentinel->first_elm_idx = HIVE_END_SENTINEL_INDEX;
    (*_hv) = (HIVE_NAME){
        .buckets      = _end_sentinel,
        .tail         = _end_sentinel,
        .end_sentinel = _end_sentinel,
        .count        = 0,
        .bucket_count = 0,
    };
    _hv->not_full_buckets.count = 0;
    _hv->not_full_buckets.cap = 16;
    _hv->not_full_buckets.array = HIVE_ALLOC_N(hive_bucket_t*, _hv->not_full_buckets.cap);
}

// TODO hive_clear

HIVE_NAME hive_clone(const HIVE_NAME * _hv)
{
    HIVE_NAME _ret;
    hive_init(&_ret);
    _ret.count = _hv->count;
    _ret.bucket_count = _hv->bucket_count;
    
    if(_hv->not_full_buckets.count > _ret.not_full_buckets.cap)
    {
        _ret.not_full_buckets.array = HIVE_REALLOC_N(_ret.not_full_buckets.array, _ret.not_full_buckets.cap, _hv->not_full_buckets.count + 1);
        _ret.not_full_buckets.cap = _hv->not_full_buckets.count + 1;
    }
    
    _ret.not_full_buckets.count = _hv->not_full_buckets.count;
    
    if(_hv->bucket_count > 0)
    {
        hive_bucket_t *_src_bucket = _hv->buckets;
        
        hive_bucket_t **_dst_bucket = &_ret.buckets;
        hive_bucket_t *_dst_prev = NULL;
        
        *_dst_bucket = (hive_bucket_t*) HIVE_ALLOC(HIVE_ALLOC_CTX, sizeof(hive_bucket_t), alignof(hive_bucket_t));
        memcpy(*_dst_bucket, _src_bucket, sizeof(hive_bucket_t));
        (*_dst_bucket)->prev = NULL;
        
        if((*_dst_bucket)->not_full_idx != UINT16_MAX)
        {
            _ret.not_full_buckets.array[(*_dst_bucket)->not_full_idx] = *_dst_bucket;
        }
        
        (*_dst_bucket)->first_elm_idx = _src_bucket->first_elm_idx;
        memcpy((*_dst_bucket)->empty_bitset, _src_bucket->empty_bitset, sizeof(_src_bucket->empty_bitset));
        
        (*_dst_bucket)->next = NULL;
        _dst_prev = (*_dst_bucket);
        _dst_bucket = &(*_dst_bucket)->next;
        _src_bucket = _src_bucket->next;
        
        while(_src_bucket != _hv->end_sentinel)
        {
            *_dst_bucket = (hive_bucket_t*) HIVE_ALLOC(HIVE_ALLOC_CTX, sizeof(hive_bucket_t), alignof(hive_bucket_t));
            memcpy(*_dst_bucket, _src_bucket, sizeof(hive_bucket_t));
            (*_dst_bucket)->next = NULL;
            (*_dst_bucket)->prev = _dst_prev;
            if((*_dst_bucket)->not_full_idx != UINT16_MAX)
            {
                _ret.not_full_buckets.array[(*_dst_bucket)->not_full_idx] = *_dst_bucket;
            }
            
            (*_dst_bucket)->first_elm_idx = _src_bucket->first_elm_idx;
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
    HIVE_TYPE *_elm_added = NULL;
    
    if(_hv->not_full_buckets.count > 0)
    {
        hive_bucket_t *_bucket = _hv->not_full_buckets.array[_hv->not_full_buckets.count - 1];
        _hv->count += 1;
        HIVE_TYPE *_elm = hive_bucket_put(_hv, _bucket, _new_elm);
        return hive_iter_to(_bucket, _elm - &_bucket->elms[0]);
    }
    
    hive_bucket_t *_new_bucket = (hive_bucket_t*) HIVE_ALLOC(HIVE_ALLOC_CTX, sizeof(hive_bucket_t), alignof(hive_bucket_t));
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
    _elm_added = hive_bucket_put(_hv, _new_bucket, _new_elm);
    _hv->count += 1;
    return hive_iter_to(_new_bucket, _elm_added - &_new_bucket->elms[0]);
}

void hive_put_all(HIVE_NAME *_hv, const HIVE_TYPE *_elms, size_t _nelms)
{
    size_t _buckets_to_fill = _nelms / HIVE_BUCKET_SIZE;
    ptrdiff_t _remaining = _nelms - (_buckets_to_fill * HIVE_BUCKET_SIZE);
    
    hive_bucket_t *_first = NULL;
    bool _first_set = false;
    hive_bucket_t *_bucket = NULL;
    hive_bucket_t *_prev = NULL;
    for(size_t _i = 0 ; _i < _buckets_to_fill ; _i++)
    {
        _bucket = (hive_bucket_t*) HIVE_ALLOC(HIVE_ALLOC_CTX, sizeof(hive_bucket_t), alignof(hive_bucket_t));
        if(!_first_set)
        {
            _first = _bucket;
            _first_set = true;
        }
        hive_bucket_init(_bucket);
        memcpy(_bucket->elms, _elms + (_i * HIVE_BUCKET_SIZE), HIVE_BUCKET_SIZE * sizeof(HIVE_TYPE));
        for(uint8_t _j = 0 ; _j <= HIVE_BUCKET_SIZE ; _j++)
        {
            _bucket->next_entries[_j].next_elm_index = _j;
        }
        _bucket->first_elm_idx = 0;
        memset(_bucket->empty_bitset, -1, 64 * 4);
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
        hive_bucket_t *_remaining_bucket = (hive_bucket_t*) HIVE_ALLOC(HIVE_ALLOC_CTX, sizeof(hive_bucket_t), alignof(hive_bucket_t));
        hive_bucket_init(_remaining_bucket);
        memcpy(_remaining_bucket->elms, _elms + (_buckets_to_fill * HIVE_BUCKET_SIZE), _remaining * sizeof(HIVE_TYPE));
        for(uint8_t _j = 0 ; _j < _remaining ; _j++)
        {
            _remaining_bucket->next_entries[_j].next_elm_index = _j;
        }
        _remaining_bucket->first_elm_idx = 0;
        // TODO set bitset
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
            
            _ret = hive_iter_to(_hv->end_sentinel, HIVE_END_SENTINEL_INDEX);
        }
        else
        {
            uint8_t _first_elm = hive_bucket_first_elm(_bucket->next);
            _ret = hive_iter_to(_bucket->next, _first_elm);
        }
        
        HIVE_FREE(HIVE_ALLOC_CTX, _bucket, sizeof(*_bucket));
        _hv->bucket_count -= 1;
    }
    else
    {
        uint8_t _next_elm = _bucket->next_entries[_index + 1].next_elm_index;
        
        if(_next_elm == HIVE_END_SENTINEL_INDEX)
        {
            hive_bucket_t *_next_bucket = _bucket->next;
            uint8_t _first_elm_in_next_bucket = hive_bucket_first_elm(_next_bucket);
            _ret = hive_iter_to(_next_bucket, _first_elm_in_next_bucket);
        }
        else
        {
            _ret = hive_iter_to(_bucket, _next_elm);
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
        (*_index) = (*_bucket)->first_elm_idx;
    }
}

void hive_foreach(HIVE_NAME *_hv, void(*_f)(HIVE_TYPE*,void*), void *_arg)
{
    hive_bucket_t *_bucket = _hv->buckets;
    uint8_t _index = _bucket->first_elm_idx;
    for( ; _bucket != _hv->end_sentinel ; hive_foreach_updater(&_index, &_bucket))
    {
        _f(&_bucket->elms[_index], _arg);
    }
}

void hive_deinit(HIVE_NAME *_hv)
{
    for(hive_bucket_t *_current = _hv->buckets ; _current != NULL ; )
    {
        hive_bucket_t *_next = _current->next;
        HIVE_FREE(HIVE_ALLOC_CTX, _current, sizeof(hive_bucket_t));
        _current = _next;
    }
    HIVE_FREE_N(_hv->not_full_buckets.array, _hv->not_full_buckets.cap);
    *_hv = (HIVE_NAME){0};
}

void hive_bucket_init(hive_bucket_t *_bucket)
{
    _bucket->first_elm_idx = HIVE_END_SENTINEL_INDEX;
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

HIVE_TYPE *hive_bucket_put(HIVE_NAME *_hv, hive_bucket_t *_bucket, HIVE_TYPE _new_elm)
{
    assert(_bucket->count < HIVE_BUCKET_SIZE);
    
    uint8_t _empty_index = hive_bucket_set_first_empty(_bucket);
    assert(_bucket->next_entries[_empty_index].next_elm_index != _empty_index);
    
    if(_bucket->next_entries[_empty_index + 1].next_elm_index != _empty_index + 1)
    {
        _bucket->next_entries[_empty_index + 1].next_elm_index = _bucket->next_entries[_empty_index].next_elm_index;
        
        uint8_t _next_elm_idx = _bucket->next_entries[_empty_index].next_elm_index;
        if(_empty_index < _bucket->first_elm_idx)
        {
            _next_elm_idx = _bucket->first_elm_idx;
        }
        if(_next_elm_idx != HIVE_END_SENTINEL_INDEX)
            _bucket->prev_entries[_next_elm_idx - 1].next_elm_index = _empty_index;
    }
    
    _bucket->elms[_empty_index] = _new_elm;
    _bucket->next_entries[_empty_index].next_elm_index = _empty_index;
    _bucket->prev_entries[_empty_index].next_elm_index = _empty_index;
    _bucket->count += 1;
    
    // TODO try not having `first_elm_idx` and instead use the bitset. might be faster, needs benchmarking.
    if(_empty_index < _bucket->first_elm_idx)
    {
        _bucket->first_elm_idx = _empty_index;
    }
    
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
    uint8_t _next_elm = _bucket->next_entries[_index + 1].next_elm_index;
    _bucket->empty_bitset[_index / 64] |= ((uint64_t)1 << (_index % 64));
    
    _bucket->prev_entries[_index].next_elm_index = _bucket->prev_entries[_index - 1].next_elm_index;
    
    _bucket->next_entries[_index].next_elm_index = _bucket->next_entries[_index + 1].next_elm_index;
    
    // TODO go the beginning of prev hole and set its skip to skip the new hole
    // same for end of next hole
    
    if(_bucket->prev_entries[_index].next_elm_index != 0)
        _bucket->next_entries[_bucket->prev_entries[_index].next_elm_index + 1].next_elm_index = _bucket->next_entries[_index].next_elm_index;
    // if(_bucket->next_entries[_index].next_elm_index != 0)
    _bucket->prev_entries[_bucket->next_entries[_index].next_elm_index - 1].next_elm_index = _bucket->prev_entries[_index].next_elm_index;
    
    if(HIVE_UNLIKELY(_index == _bucket->first_elm_idx))
    {
        _bucket->first_elm_idx = _next_elm;
        
        if(_next_elm == HIVE_END_SENTINEL_INDEX)
        {
            _hv->not_full_buckets.array[_bucket->not_full_idx] = _hv->not_full_buckets.array[_hv->not_full_buckets.count-1];
            _hv->not_full_buckets.array[_bucket->not_full_idx]->not_full_idx = _bucket->not_full_idx;
            _hv->not_full_buckets.count -= 1;
            
            _bucket->count -= 1;
            return true;
        }
    }
    
    if(HIVE_UNLIKELY(_bucket->count == HIVE_BUCKET_SIZE))
    {
        hive_push_not_full_bucket(_hv, _bucket);
    }
    
    _bucket->count -= 1;
    
    return false;
}

uint8_t hive_bucket_first_elm(const hive_bucket_t *_bucket)
{
    return _bucket->first_elm_idx;
    // if(_bucket->empty_bitset[0] != 0)
    // {
    //     return __builtin_ctzll(~_bucket->empty_bitset[0]);
    // }
    // else if(_bucket->empty_bitset[1] != 0)
    // {
    //     return __builtin_ctzll(~_bucket->empty_bitset[1]) + 64;
    // }
    // else if(_bucket->empty_bitset[1] != 0)
    // {
    //     return __builtin_ctzll(~_bucket->empty_bitset[2]) + 128;
    // }
    // else if(_bucket->empty_bitset[1] != 0)
    // {
    //     return __builtin_ctzll(~_bucket->empty_bitset[3]) + 192;
    // }
    // assert(0);
}

uint8_t hive_bucket_first_empty(const hive_bucket_t *_bucket)
{
    if(_bucket->empty_bitset[0] != 0)
    {
        return __builtin_ctzll(_bucket->empty_bitset[0]);
    }
    else if(_bucket->empty_bitset[1] != 0)
    {
        return __builtin_ctzll(_bucket->empty_bitset[1]) + 64;
    }
    else if(_bucket->empty_bitset[2] != 0)
    {
        return __builtin_ctzll(_bucket->empty_bitset[2]) + 128;
    }
    else if(_bucket->empty_bitset[3] != 0)
    {
        return __builtin_ctzll(_bucket->empty_bitset[3]) + 192;
    }
    assert(0);
}

uint8_t hive_bucket_set_first_empty(hive_bucket_t *_bucket)
{
    if(_bucket->empty_bitset[0] != 0)
    {
        int i = __builtin_ctzll(_bucket->empty_bitset[0]);
        _bucket->empty_bitset[0] &= ~((uint64_t)1 << i);
        return i;
    }
    else if(_bucket->empty_bitset[1] != 0)
    {
        int i = __builtin_ctzll(_bucket->empty_bitset[1]);
        _bucket->empty_bitset[1] &= ~((uint64_t)1 << i);
        return i + 64;
    }
    else if(_bucket->empty_bitset[2] != 0)
    {
        int i = __builtin_ctzll(_bucket->empty_bitset[2]);
        _bucket->empty_bitset[2] &= ~((uint64_t)1 << i);
        return i + 128;
    }
    else if(_bucket->empty_bitset[3] != 0)
    {
        int i = __builtin_ctzll(_bucket->empty_bitset[3]);
        _bucket->empty_bitset[3] &= ~((uint64_t)1 << i);
        return i + 192;
    }
    assert(0);
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
    return hive_iter_to(_hv->buckets, _hv->buckets->first_elm_idx);
}

hive_iter hive_end(const HIVE_NAME *_hv)
{
    return hive_iter_to(_hv->end_sentinel, HIVE_END_SENTINEL_INDEX);
}

hive_iter hive_iter_to(hive_bucket_t *_bucket, uint8_t _index)
{
    hive_iter _ret;
    _ret.bucket = _bucket;
    _ret.elm = &_bucket->elms[_index];
    _ret.next_entry = &_ret.bucket->next_entries[_index] + 1;
    return _ret;
}

bool hive_iter_eq(hive_iter _a, hive_iter _b)
{
    return (_a.elm == _b.elm);
}

hive_iter hive_iter_next(hive_iter _it)
{
    uint8_t _index = _it.next_entry->next_elm_index;
    
    if (HIVE_UNLIKELY(_index == HIVE_END_SENTINEL_INDEX))
    {
        _it.bucket = _it.bucket->next;
        _index = _it.bucket->first_elm_idx;
    }
    _it.elm = &_it.bucket->elms[_index];
    _it.next_entry = &_it.bucket->next_entries[_index] + 1;
    
    return _it;
}

HIVE_TYPE *hive_iter_elm(hive_iter _it)
{
    return _it.elm;
}

hive_iter hive_iter_del(HIVE_NAME *_hv, hive_iter _it)
{
    uint8_t _index = _it.elm - _it.bucket->elms;
    
    return hive_del_helper(_hv, _it.bucket->prev, _it.bucket, _index);
}

hive_handle hive_iter_to_handle(hive_iter _it)
{
    return (hive_handle){.bucket = _it.bucket, .elm = _it.elm};
}

hive_entry_t *hive_handle_elm(hive_handle _handle)
{
    return _handle.elm;
}

hive_iter hive_handle_del(HIVE_NAME *_hv, hive_handle _handle)
{
    uint8_t _index = _handle.elm - _handle.bucket->elms;
    
    return hive_del_helper(_hv, _handle.bucket->prev, _handle.bucket, _index);
}

#ifdef HIVE_TEST

hive_iter hive_checked_put(HIVE_NAME *_hive, HIVE_TYPE elm)
{
    if(_hive->not_full_buckets.count == 0)
        return hive_put(_hive, elm);
    
    hive_bucket_t *_bucket = _hive->not_full_buckets.array[_hive->not_full_buckets.count - 1];
    
    uint16_t _old_not_full_count = _hive->not_full_buckets.count;
    uint8_t _old_count = _bucket->count;
    bool _will_fill = _bucket->count == (HIVE_BUCKET_SIZE - 1);
    
    uint8_t _index = hive_bucket_first_empty(_bucket);
    
    hive_next_entry_t _old_nexts[HIVE_BUCKET_SIZE + 2];
    memcpy(_old_nexts, _bucket->next_entries, sizeof(_old_nexts));
    
    hive_next_entry_t _old_prevs[HIVE_BUCKET_SIZE + 2];
    memcpy(_old_prevs, _bucket->prev_entries, sizeof(_old_prevs));
    
    HIVE_TYPE *_old_elms = HIVE_ALLOC_N(HIVE_TYPE, HIVE_BUCKET_SIZE + 2);
    memcpy(_old_elms, _bucket->elms, sizeof(_bucket->elms));
    
    uint64_t _old_bitset[4];
    memcpy(_old_bitset, _bucket->empty_bitset, sizeof(_old_bitset));
    
    hive_iter _ret = hive_put(_hive, elm);
    
    assert(_old_count == _bucket->count - 1);
    if(_will_fill)
    {
        assert(_old_not_full_count == _hive->not_full_buckets.count + 1);
    }
    
    for(uint8_t i = 0 ; i < _index ; i++)
    {
        assert(_old_nexts[i].next_elm_index == _bucket->next_entries[i].next_elm_index);
        assert(i == 0 || _bucket->next_entries[i].next_elm_index == i);
    }
    assert(_old_nexts[_index].next_elm_index != _bucket->next_entries[_index].next_elm_index);
    assert(_bucket->next_entries[_index].next_elm_index == _index);
    for(int i = _index + 2 ; i < HIVE_END_SENTINEL_INDEX ; i++)
    {
        assert(_old_nexts[i].next_elm_index == _bucket->next_entries[i].next_elm_index);
    }
    
    for(uint8_t i = 0 ; i < _index ; i++)
    {
        assert(_old_prevs[i].next_elm_index == _bucket->prev_entries[i].next_elm_index);
        assert(i == 0 || _bucket->prev_entries[i].next_elm_index == i);
    }
    assert(_old_prevs[_index].next_elm_index != _bucket->prev_entries[_index].next_elm_index);
    assert(_bucket->prev_entries[_index].next_elm_index == _index);
    uint8_t _hole_after_index = _index + 1;
    for( ; _hole_after_index < HIVE_END_SENTINEL_INDEX ; _hole_after_index++)
    {
        if(_old_prevs[_hole_after_index].next_elm_index == _hole_after_index)
            break;
    }
    
    if(_hole_after_index != HIVE_END_SENTINEL_INDEX)
    {
        assert(_bucket->prev_entries[_hole_after_index - 1].next_elm_index == _index);
    }
    
    for(uint8_t i = _hole_after_index ; i < HIVE_END_SENTINEL_INDEX ; i++)
    {
        assert(_bucket->prev_entries[i].next_elm_index == _old_prevs[i].next_elm_index);
    }
    
    return _ret;
}

hive_iter hive_checked_del(HIVE_NAME *_hive, HIVE_TYPE *elm)
{
    
}

hive_iter hive_iter_checked_del(HIVE_NAME *_hive, hive_iter it)
{
    
}

#endif

#endif

#undef HIVE_TYPE
#undef HIVE_NAME
#undef HIVE_IMPL
#undef HIVE_ALLOC
#undef HIVE_FREE
#undef HIVE_REALLOC
#undef HIVE_ALLOC_CTX

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
#undef hive_bucket_put
#undef hive_bucket_del
#undef hive_bucket_is_elm_within
#undef hive_bucket_first_elm
#undef hive_iter_next
#undef hive_iter_elm
#undef hive_iter_del
#undef hive_iter_eq
#undef hive_iter_is_end
#undef hive_iter_to
#undef hive_handle
#undef hive_handle_elm
#undef hive_handle_del
#undef hive_iter_to_handle

#undef HIVE_DECLARED
