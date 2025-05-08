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

    #define SP_BUCKET_SIZE (2047)

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

#if SP_BUCKET_SIZE > 254

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

#define sp_iter_t               SP_CAT(SP_NAME, _iter_t)
#define sp_bucket_t             SP_CAT(SP_NAME, _bucket_t)
#define sp_entry_t              SP_CAT(SP_NAME, _entry_t)
#define sp_offset_entry_t     SP_CAT(SP_NAME, _offset_entry_t)
#define sp_init                 SP_CAT(SP_NAME, _init)
#define sp_put                  SP_CAT(SP_NAME, _put)
#define sp_put_all              SP_CAT(SP_NAME, _put_all)
#define sp_pop                  SP_CAT(SP_NAME, _pop)
#define sp_foreach              SP_CAT(SP_NAME, _foreach)
#define sp_foreach_updater      SP_CAT(SP_NAME, _foreach_updater)
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
                                                                                      \
#define SP_FOREACH(sp, body)                                                          \
do                                                                                    \
{                                                                                     \
    typeof(sp) sp_sp = sp;                                                            \
    if(sp_sp->bucket_count == 0)                                                      \
        break;                                                                        \
    typeof(sp_sp->buckets) bucket = sp_sp->buckets;                                   \
    typeof(sp_sp->buckets->offsets[0].next_elm_offset) index = bucket->first_elm_idx; \
    while(                                                                            \
    bucket != sp_sp->tail || index != (SP_ARR_LEN(sp_sp->buckets->offsets) - 1)       \
    )                                                                                 \
    {                                                                                 \
        body                                                                          \
                                                                                      \
        ++(index);                                                                    \
        (index) += (bucket)->offsets[index].next_elm_offset;                          \
        if(SP_UNLIKELY(index == (SP_ARR_LEN(sp_sp->buckets->offsets) - 1)))           \
        {                                                                             \
            if(SP_LIKELY((bucket)->next != NULL))                                     \
            {                                                                         \
                (bucket) = (bucket)->next;                                            \
                (index) = (bucket)->first_elm_idx;                                    \
            }                                                                         \
        }                                                                             \
    }                                                                                 \
} while(0)

#define SP_IT \
((typeof(bucket->elms[index].value)*const) &bucket->elms[index])

typedef struct sp_entry_t
{
    typeof(SP_TYPE) value;
} sp_entry_t;

typedef struct sp_offset_entry_t
{
    sp_index_t next_elm_offset;
} sp_offset_entry_t;

typedef struct sp_bucket_t
{
    sp_index_t first_elm_idx;
    sp_entry_t elms[SP_BUCKET_SIZE + 1];
    sp_offset_entry_t offsets[SP_BUCKET_SIZE + 1];
    struct sp_bucket_t *next;
} sp_bucket_t;

typedef struct SP_NAME
{
    sp_bucket_t *buckets;
    sp_bucket_t *tail;
    size_t count;
    size_t bucket_count;
} SP_NAME;

typedef struct sp_iter_t
{
    SP_NAME *sp;
    sp_bucket_t *bucket;
    sp_index_t index;
} sp_iter_t;

void sp_init(SP_NAME *sp);
SP_TYPE *sp_put(SP_NAME *sp, SP_TYPE new_elm);
void sp_put_all(SP_NAME *sp, SP_TYPE *elms, size_t nelms);
sp_iter_t sp_pop(SP_NAME *sp, SP_TYPE *elm);
void sp_foreach(const SP_NAME *sp, void(*f)(SP_TYPE*,void*), void *arg);
void sp_foreach_updater(sp_index_t *index, sp_bucket_t **bucket);
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
        for(sp_index_t j = 0 ; j < SP_BUCKET_SIZE ; j++)
        {
            bucket->offsets[j].next_elm_offset = 0;
        }
        bucket->first_elm_idx = 0;
        sp->bucket_count += 1;
        sp->count += SP_BUCKET_SIZE;
        
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
            remaining_bucket->offsets[j].next_elm_offset = 0;
        }
        remaining_bucket->first_elm_idx = 0;
        
        if(bucket)
        {
            bucket->next = remaining_bucket;
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
        sp->tail->next = first;
        sp->tail = sp->tail->next;
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
                    ret.bucket = bucket->next;
                    ret.index = sp_bucket_first_elm(ret.bucket);
                }
                else
                {
                    ret.bucket = sp->tail;
                    ret.index = SP_BUCKET_SIZE;
                }
                
                SP_FREE(SP_ALLOC_CTX, bucket, sizeof(*bucket));
                sp->bucket_count -= 1;
            }
            else
            {
                sp_index_t next_elm = elm - (SP_TYPE*)bucket->elms + 1;
                for( ; bucket->offsets[next_elm].next_elm_offset != 0 ; next_elm++)
                {
                    ;
                }
                if(next_elm == SP_BUCKET_SIZE && bucket->next != NULL)
                {
                    sp_bucket_t *next_bucket = bucket->next;
                    sp_index_t first_elm_in_next_bucket = sp_bucket_first_elm(next_bucket);
                    ret.bucket = next_bucket;
                    ret.index = first_elm_in_next_bucket;
                }
                else
                {
                    ret.bucket = bucket;
                    ret.index = next_elm;
                }
            }
            sp->count -= 1;
            return ret;
        }
        prev = bucket;
    }
    return ret;
}

void sp_foreach_updater(sp_index_t *index, sp_bucket_t **bucket)
{
    ++(*index);
    (*index) += (*bucket)->offsets[*index].next_elm_offset;
    if(*index == SP_BUCKET_SIZE)
    {
        if((*bucket)->next != NULL)
        {
            (*bucket) = (*bucket)->next;
            (*index) = (*bucket)->first_elm_idx;
        }
    }
}

void sp_foreach(const SP_NAME *sp, void(*f)(SP_TYPE*,void*), void *arg)
{
    if(sp->bucket_count == 0)
        return;
    sp_bucket_t *bucket = sp->buckets;
    sp_index_t index = bucket->first_elm_idx;
    for(
        ;
        !(bucket == sp->tail && index == SP_BUCKET_SIZE) ;
        sp_foreach_updater(&index, &bucket)
        )
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
    *sp = (SP_NAME){0};
}

void sp_bucket_init(sp_bucket_t *bucket)
{
    bucket->first_elm_idx = SP_INDEX_MAX;
    bucket->next = NULL;
    
    for(sp_index_t i = 0 ; i <= SP_BUCKET_SIZE ; i++)
    {
        bucket->offsets[i].next_elm_offset = &bucket->elms[SP_BUCKET_SIZE] - &bucket->elms[i];
    }
}

SP_TYPE *sp_bucket_put(SP_NAME *sp, sp_bucket_t *bucket, SP_TYPE new_elm)
{
    int emptyIndex = 0;
    for( ; emptyIndex < SP_BUCKET_SIZE ; emptyIndex++)
    {
        if(bucket->offsets[emptyIndex].next_elm_offset != 0)
            goto emptyIndexFound;
    }
    return NULL;
    
    emptyIndexFound:
    bucket->elms[emptyIndex].value = new_elm;
    
    bucket->offsets[emptyIndex].next_elm_offset = 0;
    
    if(emptyIndex < bucket->first_elm_idx)
    {
        bucket->first_elm_idx = emptyIndex;
        for(sp_index_t i = 0 ; i < bucket->first_elm_idx ; i++)
        {
            bucket->offsets[i].next_elm_offset = &bucket->offsets[bucket->first_elm_idx] - &bucket->offsets[i];
        }
    }
    
    return &bucket->elms[emptyIndex].value;
}

bool sp_bucket_pop(SP_NAME *sp, sp_bucket_t *bucket, SP_TYPE *elm)
{
    sp_entry_t *as_entry = (sp_entry_t*) elm;
    ptrdiff_t index = as_entry - bucket->elms;
    if(bucket->offsets[index].next_elm_offset != 0)
        return false;
    
    bool is_first_elm = false;
    sp_index_t index_of_first_elm = sp_bucket_first_elm(bucket);
    if(index == index_of_first_elm)
    {
        is_first_elm = true;
    }
    
    ptrdiff_t next_elm;
    for(next_elm = index + 1 ; next_elm < SP_BUCKET_SIZE && bucket->offsets[next_elm].next_elm_offset != 0 ; next_elm++ )
    {
        ;
    }
    
    bucket->offsets[index].next_elm_offset = &bucket->offsets[next_elm] - &bucket->offsets[index];
    
    for(ptrdiff_t i = (ptrdiff_t)index - 1 ; i >= 0 && bucket->offsets[i].next_elm_offset != 0 ; i--)
    {
        bucket->offsets[i].next_elm_offset = &bucket->offsets[next_elm] - &bucket->offsets[i];
    }
    
    bool is_empty = false;
    if(index == bucket->first_elm_idx)
    {
        for(sp_index_t j = index + 1 ; j < SP_BUCKET_SIZE ; j++)
        {
            if(bucket->offsets[j].next_elm_offset == 0)
            {
                bucket->first_elm_idx = j;
                goto not_empty;
            }
        }
        is_empty = true;
        bucket->first_elm_idx = SP_BUCKET_SIZE; // this doesn't really matter, this node will get deleted since it's empty
    }
    
    not_empty:
    
    return is_empty;
}

sp_index_t sp_bucket_first_elm(sp_bucket_t *bucket)
{
    return bucket->first_elm_idx;
}

sp_index_t sp_bucket_last_elm(sp_bucket_t *bucket)
{
    for(sp_index_t i = SP_BUCKET_SIZE - 1 ; i >= 0 ; i--)
    {
        if(bucket->offsets[i].next_elm_offset == 0)
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

// TODO cache the begin and end so it doesn't have to be checked here
// we also need sentinel buckets
sp_iter_t sp_begin(SP_NAME *sp)
{
    if(sp->count != 0)
    {
        return (sp_iter_t){
            .sp = sp,
            .bucket = sp->buckets,
            .index = sp->buckets->first_elm_idx
        };
    }
    else
    {
        return (sp_iter_t){
            .sp = sp,
            .bucket = NULL,
            .index = SP_BUCKET_SIZE
        };
    }
}

sp_iter_t sp_end(SP_NAME *sp)
{
    return (sp_iter_t){
        .sp = sp,
        .bucket = sp->tail,
        .index = SP_BUCKET_SIZE,
    };
}

sp_iter_t sp_iter_next(sp_iter_t it)
{
    it.index += 1;
    it.index += it.bucket->offsets[it.index].next_elm_offset;
    if(it.index == SP_BUCKET_SIZE)
    {
        if(it.bucket->next != NULL)
        {
            it.bucket = it.bucket->next;
            it.index = it.bucket->first_elm_idx;
        }
    }
    return it;
}

SP_TYPE *sp_iter_elm(sp_iter_t it)
{
    return &it.bucket->elms[it.index].value;
}

sp_iter_t sp_iter_pop(sp_iter_t it)
{
    return sp_pop(it.sp, &it.bucket->elms[it.index].value);
}

bool sp_iter_eq(sp_iter_t a, sp_iter_t b)
{
    return (a.bucket == b.bucket) && (a.index == b.index);
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
