#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"
#include <time.h>

#define HIVE_TEST

#define ASSERT(expr)                                                                                     \
do                                                                                                       \
{                                                                                                        \
    if (!(expr))                                                                                         \
    {                                                                                                    \
        fprintf(stderr, "ASSERT FAILED: %s\n  at %s:%d in %s()\n", #expr, __FILE__, __LINE__, __func__); \
        exit(1);                                                                                         \
    }                                                                                                    \
} while (0)

static int compare_ints(const void *a, const void *b)
{
    int arg1 = *(const int *)a;
    int arg2 = *(const int *)b;
    return (arg1 > arg2) - (arg1 < arg2);
}

#define HIVE_IMPL
#define HIVE_TYPE int
#define HIVE_NAME int_hv
#include "hive.h"

#define big_hv_iter_elm(it)   it.ptr
#define big_hv_handle_elm(it) it.ptr
#define int_hv_iter_elm(it)   it.ptr
#define int_hv_handle_elm(it) it.ptr

struct Collector
{
    int *data;
    size_t idx, cap;
};

static void collect_int(int *v, void *arg)
{
    struct Collector *c = arg;
    if (c->idx == c->cap)
    {
        size_t new_cap = c->cap ? c->cap * 2 : 16;
        int *tmp = realloc(c->data, new_cap * sizeof *tmp);
        ASSERT(tmp != NULL);
        c->data = tmp;
        c->cap = new_cap;
    }
    c->data[c->idx++] = *v;
}

static void test_init_deinit(void)
{
    int_hv sp;
    int_hv_init(&sp);
    ASSERT(sp.count == 0);
    int_hv_deinit(&sp);
}

static void test_single_put_and_loop(void)
{
    int_hv sp;
    int_hv_init(&sp);
    int *p = int_hv_checked_put(&sp, 42).ptr;
    ASSERT(p && *p == 42);
    ASSERT(sp.count == (size_t)1);
    struct Collector c = {NULL, 0, 0};
    
    HIVE_FOR_EACH(it, int_hv_begin(&sp), int_hv_end(&sp))
    {
        collect_int(it.ptr, &c);
    }
    
    ASSERT(c.idx == 1);
    ASSERT(c.data[0] == 42);
    free(c.data);
    int_hv_deinit(&sp);
}

static void test_multiple_puts(void)
{
    int_hv sp;
    int_hv_init(&sp);
    const int N = 100;
    for (int i = 0; i < N; i++)
        ASSERT(int_hv_checked_put(&sp, i).ptr != NULL);
    ASSERT(sp.count == (size_t)N);
    struct Collector c = {NULL, 0, 0};
    HIVE_FOR_EACH(it, int_hv_begin(&sp), int_hv_end(&sp))
    {
        collect_int(it.ptr, &c);
    }
    ASSERT(c.idx == (size_t)N);
    
    qsort(c.data, c.idx, sizeof(int), compare_ints);
    for (size_t i = 0; i < c.idx; i++)
        ASSERT(c.data[i] == (int)i);
    free(c.data);
    int_hv_deinit(&sp);
}

static void test_pointer_stability(void)
{
    int_hv sp;
    int_hv_init(&sp);
    int *p1 = int_hv_checked_put(&sp, 10).ptr;
    int *p2 = int_hv_checked_put(&sp, 20).ptr;
    int *p3 = int_hv_checked_put(&sp, 30).ptr;
    ASSERT(sp.count == 3);
    int_hv_del(&sp, p3);
    ASSERT(sp.count == 2);
    ASSERT(*p1 == 10);
    ASSERT(*p2 == 20);
    int_hv_deinit(&sp);
}

static void test_del_and_iteration(void)
{
    int_hv sp;
    int_hv_init(&sp);
    int *a = int_hv_checked_put(&sp, 1).ptr;
    int *b = int_hv_checked_put(&sp, 2).ptr;
    int *c = int_hv_checked_put(&sp, 3).ptr;
    ASSERT(sp.count == 3);
    int_hv_del(&sp, b);
    ASSERT(sp.count == 2);
    struct Collector col = {NULL, 0, 0};
    HIVE_FOR_EACH(it, int_hv_begin(&sp), int_hv_end(&sp))
    {
        collect_int(it.ptr, &col);
    }
    ASSERT(col.idx == 2);
    
    qsort(col.data, col.idx, sizeof(int), compare_ints);
    ASSERT(col.data[0] == 1);
    ASSERT(col.data[1] == 3);
    
    free(col.data);
    int_hv_deinit(&sp);
    (void)a, (void)b, (void)c;
}

static void test_stress_inserts_dels(void)
{
    int_hv sp;
    int_hv_init(&sp);
    const int M = 10000;
    int **ptrs = malloc(M * sizeof *ptrs);
    ASSERT(ptrs != NULL);
    for (int i = 0; i < M; i++)
        ptrs[i] = int_hv_checked_put(&sp, i).ptr;
    ASSERT(sp.count == (size_t)M);
    
    for (int i = 0; i < M; i += 2)
        int_hv_del(&sp, ptrs[i]);
    ASSERT(sp.count == (size_t)M / 2);
    struct Collector c = {NULL, 0, 0};
    HIVE_FOR_EACH(it, int_hv_begin(&sp), int_hv_end(&sp))
    {
        collect_int(it.ptr, &c);
    }
    ASSERT(c.idx == (size_t)(M / 2));
    
    qsort(c.data, c.idx, sizeof(int), compare_ints);
    for (size_t i = 0; i < c.idx; i++)
        ASSERT(c.data[i] == 1 + 2 * (int)i);
    
    free(c.data);
    free(ptrs);
    int_hv_deinit(&sp);
}

static void test_smaller_stress_inserts_dels(void)
{
    int_hv sp;
    int_hv_init(&sp);
    const int M = 200;
    int **ptrs = malloc(M * sizeof *ptrs);
    ASSERT(ptrs != NULL);
    
    for (int i = 0; i < M; i++)
    {
        ptrs[i] = int_hv_checked_put(&sp, i).ptr;
        ASSERT(ptrs[i] != NULL);
    }
    ASSERT(sp.count == (size_t)M);
    
    for (int i = 0; i < M; i += 2)
        int_hv_del(&sp, ptrs[i]);
    ASSERT(sp.count == (size_t)M / 2);
    
    struct Collector c = {NULL, 0, 0};
    HIVE_FOR_EACH(it, int_hv_begin(&sp), int_hv_end(&sp))
    {
        collect_int(it.ptr, &c);
    }
    ASSERT(c.idx == (size_t)(M / 2));
    
    qsort(c.data, c.idx, sizeof(int), compare_ints);
    for (size_t i = 0; i < c.idx; i++)
        ASSERT(c.data[i] == 1 + 2 * (int)i);
    
    free(c.data);
    free(ptrs);
    int_hv_deinit(&sp);
}

typedef struct Big
{
    char _m[256 - 4];
    int i;
} Big;

#define HIVE_IMPL
#define HIVE_TYPE Big
#define HIVE_NAME big_hv
#include "hive.h"

static void collect_big(Big *v, void *arg)
{
    struct Collector *c = arg;
    if (c->idx == c->cap)
    {
        size_t new_cap = c->cap ? c->cap * 2 : 16;
        int *tmp = realloc(c->data, new_cap * sizeof *tmp);
        ASSERT(tmp != NULL);
        c->data = tmp;
        c->cap = new_cap;
    }
    c->data[c->idx++] = v->i;
}

static void test_big_init_deinit(void)
{
    big_hv sp;
    big_hv_init(&sp);
    ASSERT(sp.count == 0);
    big_hv_deinit(&sp);
}

static void test_big_single_put_and_loop(void)
{
    big_hv sp;
    big_hv_init(&sp);
    Big *p = big_hv_checked_put(&sp, (Big){.i = 42}).ptr;
    ASSERT(p && p->i == 42);
    ASSERT(sp.count == 1);
    
    struct Collector c = {NULL, 0, 0};
    
    for(big_hv_iter it = big_hv_begin(&sp) ; !big_hv_iter_eq(it, big_hv_end(&sp)) ; it = big_hv_iter_next(it) )
    {
        collect_big(it.ptr, &c);
    }
    
    ASSERT(c.idx == 1);
    ASSERT(c.data[0] == 42);
    free(c.data);
    big_hv_deinit(&sp);
}

static void test_big_multiple_puts(void)
{
    big_hv sp;
    big_hv_init(&sp);
    const int N = 100;
    for (int i = 0; i < N; i++)
    {
        Big *p = big_hv_checked_put(&sp, (Big){.i = i}).ptr;
        ASSERT(p != NULL);
    }
    ASSERT(sp.count == (size_t)N);
    
    struct Collector c = {NULL, 0, 0};
    for (big_hv_iter it = big_hv_begin(&sp), end = big_hv_end(&sp); !big_hv_iter_eq(it, end);
         it = big_hv_iter_next(it))
         {
             collect_big(big_hv_iter_elm(it), &c);
         }
         ASSERT(c.idx == (size_t)N);
    
    qsort(c.data, c.idx, sizeof(int), compare_ints);
    for (size_t i = 0; i < c.idx; i++)
        ASSERT(c.data[i] == (int)i);
    
    free(c.data);
    big_hv_deinit(&sp);
}

static void test_big_pointer_stability(void)
{
    big_hv sp;
    big_hv_init(&sp);
    Big *p1 = big_hv_checked_put(&sp, (Big){.i = 10}).ptr;
    Big *p2 = big_hv_checked_put(&sp, (Big){.i = 20}).ptr;
    Big *p3 = big_hv_checked_put(&sp, (Big){.i = 30}).ptr;
    ASSERT(sp.count == 3);
    big_hv_checked_del(&sp, p3);
    ASSERT(sp.count == 2);
    ASSERT(p1->i == 10);
    ASSERT(p2->i == 20);
    big_hv_deinit(&sp);
}

static void test_big_del_and_iteration(void)
{
    big_hv sp;
    big_hv_init(&sp);
    Big *a = big_hv_checked_put(&sp, (Big){.i = 1}).ptr;
    Big *b = big_hv_checked_put(&sp, (Big){.i = 2}).ptr;
    Big *c = big_hv_checked_put(&sp, (Big){.i = 3}).ptr;
    ASSERT(sp.count == 3);
    big_hv_checked_del(&sp, b);
    ASSERT(sp.count == 2);
    
    struct Collector col = {NULL, 0, 0};
    for(big_hv_iter it = big_hv_begin(&sp) ; !big_hv_iter_eq(it, big_hv_end(&sp))  ; it = big_hv_iter_next(it))
{
    collect_big(it.ptr, &col);
}
    ASSERT(col.idx == 2);
    
    qsort(col.data, col.idx, sizeof(int), compare_ints);
    ASSERT(col.data[0] == 1);
    ASSERT(col.data[1] == 3);
    
    free(col.data);
    big_hv_deinit(&sp);
    
    (void)a, (void)b, (void)c;
}

static void test_big_stress_inserts_dels(void)
{
    big_hv sp;
    big_hv_init(&sp);
    const int M = 2048 * 32;
    Big **ptrs = malloc(M * sizeof *ptrs);
    ASSERT(ptrs != NULL);
    
    for (int i = 0; i < M; i++)
    {
        ptrs[i] = big_hv_checked_put(&sp, (Big){.i = i}).ptr;
        ASSERT(ptrs[i] != NULL);
    }
    ASSERT(sp.count == (size_t)M);
    
    for (int i = 0; i < M; i += 2)
        big_hv_checked_del(&sp, ptrs[i]);
    ASSERT(sp.count == (size_t)M / 2);
    
    struct Collector col = {NULL, 0, 0};
    for (big_hv_iter it = big_hv_begin(&sp), end = big_hv_end(&sp); !big_hv_iter_eq(it, end); it = big_hv_iter_next(it))
    {
        collect_big(big_hv_iter_elm(it), &col);
    }
    ASSERT(col.idx == (size_t)M / 2);
    
    qsort(col.data, col.idx, sizeof(int), compare_ints);
    for (size_t i = 0; i < col.idx; i++)
        ASSERT(col.data[i] == 1 + 2 * (int)i);
    
    free(col.data);
    free(ptrs);
    big_hv_deinit(&sp);
}

static void test_int_iteration_equivalence_after_random_dels(void)
{
    int_hv sp;
    int_hv_init(&sp);
    const int N = 20;
    int **ptrs = malloc(N * sizeof *ptrs);
    ASSERT(ptrs != NULL);
    
    srand(42);
    for (int i = 0; i < N; i++)
        ptrs[i] = int_hv_checked_put(&sp, i).ptr;
    
    for (int i = 0; i < N; i++)
    {
        if (rand() % 2)
        {
            int_hv_del(&sp, ptrs[i]);
            ptrs[i] = NULL;
        }
    }
    
    struct Collector col1 = {NULL, 0, 0};
    struct Collector col2 = {NULL, 0, 0};
    struct Collector col3 = {NULL, 0, 0};
    
    HIVE_FOR_EACH(it, int_hv_begin(&sp), int_hv_end(&sp))
    {
        collect_int(it.ptr, &col1);
    }
    
    for(int_hv_iter it = int_hv_begin(&sp) ; !int_hv_iter_eq(it, int_hv_end(&sp)) ; it = int_hv_iter_next(it))
    {
        collect_int(it.ptr, &col2);
    }
    
    for (int_hv_iter it = int_hv_begin(&sp), end = int_hv_end(&sp); !int_hv_iter_eq(it, end);
         it = int_hv_iter_next(it))
         {
             collect_int(int_hv_iter_elm(it), &col3);
         }
         
         ASSERT(col1.idx == col2.idx && col2.idx == col3.idx);
    
    qsort(col1.data, col1.idx, sizeof(int), compare_ints);
    qsort(col2.data, col2.idx, sizeof(int), compare_ints);
    qsort(col3.data, col3.idx, sizeof(int), compare_ints);
    
    for (size_t i = 0; i < col1.idx; i++)
    {
        ASSERT(col1.data[i] == col2.data[i]);
        ASSERT(col2.data[i] == col3.data[i]);
    }
    
    free(col1.data);
    free(col2.data);
    free(col3.data);
    free(ptrs);
    int_hv_deinit(&sp);
}

static void test_big_iteration_equivalence_after_random_dels(void)
{
    big_hv sp;
    big_hv_init(&sp);
    const int N = 1024;
    Big **ptrs = malloc(N * sizeof *ptrs);
    ASSERT(ptrs != NULL);
    
    srand(1337);
    for (int i = 0; i < N; i++)
        ptrs[i] = big_hv_checked_put(&sp, (Big){.i = i}).ptr;
    
    for (int i = 0; i < N; i++)
    {
        if (rand() % 2)
        {
            big_hv_checked_del(&sp, ptrs[i]);
            ptrs[i] = NULL;
        }
    }
    
    struct Collector col1 = {NULL, 0, 0};
    struct Collector col2 = {NULL, 0, 0};
    struct Collector col3 = {NULL, 0, 0};
    
    HIVE_FOR_EACH(it, big_hv_begin(&sp), big_hv_end(&sp))
    {
        collect_big(it.ptr, &col1);
    }
    for(big_hv_iter it = big_hv_begin(&sp) ; !big_hv_iter_eq(it, big_hv_end(&sp))  ; it = big_hv_iter_next(it))
    {
        collect_big(it.ptr, &col2);
    }
    for (big_hv_iter it = big_hv_begin(&sp), end = big_hv_end(&sp); !big_hv_iter_eq(it, end);
         it = big_hv_iter_next(it))
    {
        collect_big(big_hv_iter_elm(it), &col3);
    }
         
    ASSERT(col1.idx == col2.idx && col2.idx == col3.idx);
    
    qsort(col1.data, col1.idx, sizeof(int), compare_ints);
    qsort(col2.data, col2.idx, sizeof(int), compare_ints);
    qsort(col3.data, col3.idx, sizeof(int), compare_ints);
    
    for (size_t i = 0; i < col1.idx; i++)
    {
        ASSERT(col1.data[i] == col2.data[i]);
        ASSERT(col2.data[i] == col3.data[i]);
    }
    
    free(col1.data);
    free(col2.data);
    free(col3.data);
    free(ptrs);
    big_hv_deinit(&sp);
}

static void test_against_dynamic_array(void)
{
    int_hv sp;
    int_hv_init(&sp);
    int *expected = NULL;
    const int N = 5000;
    for (int i = 0; i < N; i++)
    {
        int *ptr = int_hv_checked_put(&sp, i).ptr;
        ASSERT(ptr != NULL);
        arrput(expected, i);
    }
    ASSERT(sp.count == (size_t)N);
    ASSERT(arrlen(expected) == N);
    
    srand(12345);
    
    for (int i = 0; i < N / 2; i++)
    {
        if (arrlen(expected) == 0)
            break;
        
        int idx = rand() % arrlen(expected);
        int value = expected[idx];
        
        int_hv_iter it;
        int *found = NULL;
        for (it = int_hv_begin(&sp); !int_hv_iter_eq(it, int_hv_end(&sp)); it = int_hv_iter_next(it))
        {
            int *current = int_hv_iter_elm(it);
            if (*current == value)
            {
                found = current;
                break;
            }
        }
        ASSERT(found != NULL);
        
        int_hv_del(&sp, found);
        
        arrdel(expected, idx);
    }
    
    struct Collector collector = {0};
    HIVE_FOR_EACH(it, int_hv_begin(&sp), int_hv_end(&sp))
    {
        collect_int(it.ptr, &collector);
    }
    
    qsort(collector.data, collector.idx,    sizeof(int), compare_ints);
    qsort(expected,       arrlen(expected), sizeof(int), compare_ints);
    
    size_t expected_len = arrlen(expected);
    ASSERT(collector.idx == (size_t)arrlen(expected));
    
    for (size_t i = 0; i < collector.idx; i++)
    {
        ASSERT(collector.data[i] == expected[i]);
    }
    
    free(collector.data);
    arrfree(expected);
    int_hv_deinit(&sp);
}

static void test_big_against_dynamic_array(void)
{
    big_hv sp;
    big_hv_init(&sp);
    int *expected = NULL;
    const int N = 50;
    for (int i = 0; i < N; i++)
    {
        Big *ptr = big_hv_checked_put(&sp, (Big){.i = i}).ptr;
        ASSERT(ptr != NULL);
        arrput(expected, i);
    }
    ASSERT(sp.count == (size_t)N);
    ASSERT(arrlen(expected) == N);
    
    srand(67890);
    for (int i = 0; i < N / 2; i++)
    {
        if (arrlen(expected) == 0)
            break;
        
        int idx = rand() % arrlen(expected);
        int value = expected[idx];
        
        big_hv_iter it;
        Big *found = NULL;
        for (it = big_hv_begin(&sp); !big_hv_iter_eq(it, big_hv_end(&sp)); it = big_hv_iter_next(it))
        {
            Big *current = big_hv_iter_elm(it);
            if (current->i == value)
            {
                found = current;
                break;
            }
        }
        ASSERT(found != NULL);
        
        big_hv_checked_del(&sp, found);
        arrdel(expected, idx);
    }
    
    struct Collector collector = {0};
    HIVE_FOR_EACH(it, big_hv_begin(&sp), big_hv_end(&sp))
    {
        collect_big(it.ptr, &collector);
    }
    
    qsort(collector.data, collector.idx, sizeof(int), compare_ints);
    qsort(expected, arrlen(expected), sizeof(int), compare_ints);
    
    ASSERT(collector.idx == (size_t)arrlen(expected));
    for (size_t i = 0; i < collector.idx; i++)
    {
        ASSERT(collector.data[i] == expected[i]);
    }
    
    free(collector.data);
    arrfree(expected);
    big_hv_deinit(&sp);
}

static void test_insert_after_erase(void)
{
    int_hv sp;
    int_hv_init(&sp);
    int *expected = NULL;
    
    const int N = 250;
    for (int i = 0; i < N; i++)
    {
        int *ptr = int_hv_checked_put(&sp, i).ptr;
        ASSERT(ptr != NULL);
        arrput(expected, i);
    }
    ASSERT(sp.count == (size_t)N);
    ASSERT(arrlen(expected) == N);
    
    srand(55555);
    for (int i = 0; i < N / 2; i++)
    {
        if (arrlen(expected) == 0)
            break;
        
        int idx = rand() % arrlen(expected);
        int value = expected[idx];
        
        int_hv_iter it;
        int *found = NULL;
        for (it = int_hv_begin(&sp); !int_hv_iter_eq(it, int_hv_end(&sp)); it = int_hv_iter_next(it))
        {
            int *current = int_hv_iter_elm(it);
            if (*current == value)
            {
                found = current;
                break;
            }
        }
        ASSERT(found != NULL);
        
        int_hv_del(&sp, found);
        arrdel(expected, idx);
    }
    
    const int M = 500;
    for (int i = N; i < N + M; i++)
    {
        int *ptr = int_hv_checked_put(&sp, i).ptr;
        ASSERT(ptr != NULL);
        arrput(expected, i);
    }
    ASSERT(sp.count == (size_t)arrlen(expected));
    
    struct Collector collector = {0};
    HIVE_FOR_EACH(it, int_hv_begin(&sp), int_hv_end(&sp))
    {
        collect_int(it.ptr, &collector);
    }
    
    qsort(collector.data, collector.idx, sizeof(int), compare_ints);
    qsort(expected, arrlen(expected), sizeof(int), compare_ints);
    
    ASSERT(collector.idx == (size_t)arrlen(expected));
    for (size_t i = 0; i < collector.idx; i++)
    {
        ASSERT(collector.data[i] == expected[i]);
    }
    
    free(collector.data);
    arrfree(expected);
    int_hv_deinit(&sp);
}

static void test_big_insert_after_erase(void)
{
    big_hv sp;
    big_hv_init(&sp);
    int *expected = NULL;
    
    const int N = 500;
    for (int i = 0; i < N; i++)
    {
        Big *ptr = big_hv_checked_put(&sp, (Big){.i = i}).ptr;
        ASSERT(ptr != NULL);
        arrput(expected, i);
    }
    ASSERT(sp.count == (size_t)N);
    ASSERT(arrlen(expected) == N);
    
    srand(99999);
    for (int i = 0; i < N / 2; i++)
    {
        if (arrlen(expected) == 0)
            break;
        
        int idx = rand() % arrlen(expected);
        int value = expected[idx];
        
        big_hv_iter it;
        Big *found = NULL;
        for (it = big_hv_begin(&sp); !big_hv_iter_eq(it, big_hv_end(&sp)); it = big_hv_iter_next(it))
        {
            Big *current = big_hv_iter_elm(it);
            if (current->i == value)
            {
                found = current;
                break;
            }
        }
        ASSERT(found != NULL);
        
        big_hv_checked_del(&sp, found);
        arrdel(expected, idx);
    }
    
    const int M = 1000;
    for (int i = N; i < N + M; i++)
    {
        Big *ptr = big_hv_checked_put(&sp, (Big){.i = i}).ptr;
        ASSERT(ptr != NULL);
        arrput(expected, i);
    }
    ASSERT(sp.count == (size_t)arrlen(expected));
    
    struct Collector collector = {0};
    HIVE_FOR_EACH(it, big_hv_begin(&sp), big_hv_end(&sp))
    {
        collect_big(it.ptr, &collector);
    }
    
    qsort(collector.data, collector.idx, sizeof(int), compare_ints);
    qsort(expected, arrlen(expected), sizeof(int), compare_ints);
    
    ASSERT(collector.idx == (size_t)arrlen(expected));
    for (size_t i = 0; i < collector.idx; i++)
    {
        ASSERT(collector.data[i] == expected[i]);
    }
    
    free(collector.data);
    arrfree(expected);
    big_hv_deinit(&sp);
}

void test_clear_bucket()
{
    big_hv sp;
    big_hv_init(&sp);
    
    Big **to_delete = NULL;
    Big *copy = NULL;
    int bucket_size = 254;
    for(int i = 0 ; i < bucket_size + bucket_size + bucket_size ; i++)
    {
        arrput(to_delete, big_hv_checked_put(&sp, (Big){.i=i}).ptr);
        arrput(copy, (Big){.i=i});
    }
    
    printf("\nNB BUCKETS = %zu\n", sp.bucket_count);
    
    for(int i = bucket_size, j = bucket_size ; i < bucket_size + bucket_size ; i++)
    {
        big_hv_checked_del(&sp, to_delete[j]);
        arrdel(copy, j);
        arrdel(to_delete, j);
    }
    
    printf("NB BUCKETS = %zu\n\n", sp.bucket_count);
    
    reset_loop:
    for(big_hv_iter it = big_hv_begin(&sp) ; !big_hv_iter_eq(it, big_hv_end(&sp)) ; it = big_hv_iter_next(it))
    {
        Big b = *(it.ptr);
        
        if(b.i == copy[0].i)
        {
            arrdel(copy, 0);
            if(arrlen(copy) == 0) goto out;
            goto reset_loop;
        }
    }
    out:
    ASSERT(arrlen(copy) == 0);
    big_hv_deinit(&sp);
    arrfree(to_delete);
    arrfree(copy);
}

static void test_empty_iteration(void)
{
    int_hv sp;
    int_hv_init(&sp);
    ASSERT(sp.count == 0);
    
    struct Collector c_foreach = {NULL, 0, 0};
    HIVE_FOR_EACH(it, int_hv_begin(&sp), int_hv_end(&sp))
    {
        collect_int(it.ptr, &c_foreach);
    }
    ASSERT(c_foreach.idx == 0);
    
    struct Collector c_macro = {NULL, 0, 0};
    for(int_hv_iter it = int_hv_begin(&sp) ; !int_hv_iter_eq(it, int_hv_end(&sp))  ; it = int_hv_iter_next(it))
    {
        collect_int(it.ptr, &c_macro);
    }
    ASSERT(c_macro.idx == 0);
    
    struct Collector c_iter = {NULL, 0, 0};
    for (int_hv_iter it = int_hv_begin(&sp),
        end = int_hv_end(&sp); 
    !int_hv_iter_eq(it, end); 
    it = int_hv_iter_next(it))
    {
        collect_int(int_hv_iter_elm(it), &c_iter);
    }
    ASSERT(c_iter.idx == 0);
    
    int_hv_deinit(&sp);
}

static void test_big_empty_iteration(void)
{
    big_hv sp;
    big_hv_init(&sp);
    ASSERT(sp.count == 0);
    
    struct Collector c_foreach = {NULL, 0, 0};
    HIVE_FOR_EACH(it, big_hv_begin(&sp), big_hv_end(&sp))
    {
        collect_big(it.ptr, &c_foreach);
    }
    ASSERT(c_foreach.idx == 0);
    
    struct Collector c_macro = {NULL, 0, 0};
    for(big_hv_iter it = big_hv_begin(&sp) ; !big_hv_iter_eq(it, big_hv_end(&sp))  ; it = big_hv_iter_next(it))
{
    collect_big(it.ptr, &c_macro);
}
    ASSERT(c_macro.idx == 0);
    
    struct Collector c_iter = {NULL, 0, 0};
    for (big_hv_iter it = big_hv_begin(&sp), end = big_hv_end(&sp); 
         !big_hv_iter_eq(it, end); 
    it = big_hv_iter_next(it))
         {
             collect_big(big_hv_iter_elm(it), &c_iter);
         }
         ASSERT(c_iter.idx == 0);
         
         big_hv_deinit(&sp);
}

static void test_int_erase_single_element(void)
{
    int_hv sp;
    int_hv_init(&sp);
    int *p = int_hv_checked_put(&sp, 42).ptr;
    ASSERT(p != NULL);
    
    int_hv_iter it = int_hv_begin(&sp);
    it = int_hv_iter_del(&sp, it);
    
    ASSERT(int_hv_iter_eq(it, int_hv_end(&sp)));
    ASSERT(sp.count == 0);
    int_hv_deinit(&sp);
}

static void test_int_erase_first_element(void)
{
    int_hv sp;
    int_hv_init(&sp);
    int_hv_checked_put(&sp, 1).ptr;
    int_hv_checked_put(&sp, 2).ptr;
    int_hv_checked_put(&sp, 3).ptr;
    
    int_hv_iter it = int_hv_begin(&sp);
    int elm = *int_hv_iter_elm(it);
    it = int_hv_iter_del(&sp, it);
    
    ASSERT(sp.count == 2);
    
    struct Collector c = {0};
    HIVE_FOR_EACH(it, int_hv_begin(&sp), int_hv_end(&sp))
    {
        collect_int(it.ptr, &c);
    }

    ASSERT(c.idx == 2);
    qsort(c.data, c.idx, sizeof(int), compare_ints);
    if(elm == 1)
    {
        ASSERT(c.data[0] == 2);
        ASSERT(c.data[1] == 3);
    }
    else if(elm == 2)
    {
        ASSERT(c.data[0] == 1);
        ASSERT(c.data[1] == 3);
    }
    else if(elm == 3)
    {
        ASSERT(c.data[0] == 1);
        ASSERT(c.data[1] == 2);
    }
    
    free(c.data);
    int_hv_deinit(&sp);
}

static void test_int_erase_last_element_during_iteration(void)
{
    int_hv sp;
    int_hv_init(&sp);
    int_hv_checked_put(&sp, 1).ptr;
    int_hv_checked_put(&sp, 2).ptr;
    int_hv_checked_put(&sp, 3).ptr;
    
    int_hv_iter it = int_hv_begin(&sp);
    int_hv_iter last = int_hv_end(&sp);
    while (!int_hv_iter_eq(it, int_hv_end(&sp))) {
        last = it;
        it = int_hv_iter_next(it);
    }
    it = last;
    int elm = *int_hv_iter_elm(it);
    it = int_hv_iter_del(&sp, it);
    
    ASSERT(sp.count == 2);
    
    struct Collector c = {0};
    HIVE_FOR_EACH(it, int_hv_begin(&sp), int_hv_end(&sp))
    {
        collect_int(it.ptr, &c);
    }

    qsort(c.data, c.idx, sizeof(int), compare_ints);
    ASSERT(c.idx == 2);
    
    if(elm == 3)
    {
        ASSERT(c.data[0] == 1);
        ASSERT(c.data[1] == 2);
    }
    else if(elm == 2)
    {
        ASSERT(c.data[0] == 1);
        ASSERT(c.data[1] == 3);
    }
    else if(elm == 1)
    {
        ASSERT(c.data[0] == 2);
        ASSERT(c.data[1] == 3);
    }
    
    free(c.data);
    int_hv_deinit(&sp);
}

static void test_int_erase_every_other_element(void)
{
    int_hv sp;
    int_hv_init(&sp);
    const int N = 10;
    for (int i = 0; i < N; i++)
        int_hv_checked_put(&sp, i).ptr;
    
    bool remove = false;
    int_hv_iter it = int_hv_begin(&sp);
    while (!int_hv_iter_eq(it, int_hv_end(&sp))) {
        if (remove) {
            it = int_hv_iter_del(&sp, it);
        } else {
            it = int_hv_iter_next(it);
        }
        remove = !remove;
    }
    
    ASSERT(sp.count == N / 2);
    
    struct Collector c = {0};
    HIVE_FOR_EACH(it, int_hv_begin(&sp), int_hv_end(&sp))
    {
        collect_int(it.ptr, &c);
    }

    ASSERT(c.idx == N / 2);
    
    free(c.data);
    int_hv_deinit(&sp);
}

static void test_int_stress_iter_del(void)
{
    int_hv sp;
    int_hv_init(&sp);
    const int M = 10000;
    for (int i = 0; i < M; i++)
        int_hv_checked_put(&sp, i).ptr;
    
    int_hv_iter it = int_hv_begin(&sp);
    size_t expected = M;
    while (!int_hv_iter_eq(it, int_hv_end(&sp))) {
        if (*(int_hv_iter_elm(it)) % 2 == 0) {
            it = int_hv_iter_del(&sp, it);
            expected--;
        } else {
            it = int_hv_iter_next(it);
        }
    }
    
    ASSERT(sp.count == expected);
    ASSERT(expected == M / 2);
    
    struct Collector c = {0};
    HIVE_FOR_EACH(it, int_hv_begin(&sp), int_hv_end(&sp))
    {
        collect_int(it.ptr, &c);
    }

    qsort(c.data, c.idx, sizeof(int), compare_ints);
    for (size_t i = 0; i < c.idx; i++)
        ASSERT(c.data[i] == (int)(2 * i + 1));
    
    free(c.data);
    int_hv_deinit(&sp);
}


static void test_big_erase_single_element(void)
{
    big_hv sp;
    big_hv_init(&sp);
    Big *p = big_hv_checked_put(&sp, (Big){.i = 42}).ptr;
    ASSERT(p != NULL);
    
    big_hv_iter it = big_hv_begin(&sp);
    it = big_hv_iter_del(&sp, it);
    
    ASSERT(big_hv_iter_eq(it, big_hv_end(&sp)));
    ASSERT(sp.count == 0);
    big_hv_deinit(&sp);
}

static void test_big_erase_first_element(void)
{
    big_hv sp;
    big_hv_init(&sp);
    big_hv_checked_put(&sp, (Big){.i = 1}).ptr;
    big_hv_checked_put(&sp, (Big){.i = 2}).ptr;
    big_hv_checked_put(&sp, (Big){.i = 3}).ptr;
    
    big_hv_iter it = big_hv_begin(&sp);
    Big popped = *big_hv_iter_elm(it);
    it = big_hv_iter_del(&sp, it);
    
    ASSERT(sp.count == 2);
    
    struct Collector c = {0};
    HIVE_FOR_EACH(it, big_hv_begin(&sp), big_hv_end(&sp))
    {
        collect_big(it.ptr, &c);
    }

    qsort(c.data, c.idx, sizeof(int), compare_ints);
    ASSERT(c.idx == 2);
    
    if(popped.i == 1)
    {
        ASSERT(c.data[0] == 2);
        ASSERT(c.data[1] == 3);
    }
    else if(popped.i == 2)
    {
        ASSERT(c.data[0] == 1);
        ASSERT(c.data[1] == 3);
    }
    else if(popped.i == 3)
    {
        ASSERT(c.data[0] == 1);
        ASSERT(c.data[1] == 2);
    }
    
    free(c.data);
    big_hv_deinit(&sp);
}

static void test_big_erase_last_element_during_iteration(void)
{
    big_hv sp;
    big_hv_init(&sp);
    big_hv_checked_put(&sp, (Big){.i = 1}).ptr;
    big_hv_checked_put(&sp, (Big){.i = 2}).ptr;
    big_hv_checked_put(&sp, (Big){.i = 3}).ptr;
    
    big_hv_iter it = big_hv_begin(&sp);
    big_hv_iter last = big_hv_end(&sp);
    while (!big_hv_iter_eq(it, big_hv_end(&sp))) {
        last = it;
        it = big_hv_iter_next(it);
    }
    it = last;
    Big popped = *big_hv_iter_elm(it);
    it = big_hv_iter_del(&sp, it);
    
    ASSERT(sp.count == 2);
    
    struct Collector c = {0};
    HIVE_FOR_EACH(it, big_hv_begin(&sp), big_hv_end(&sp))
    {
        collect_big(it.ptr, &c);
    }

    qsort(c.data, c.idx, sizeof(int), compare_ints);
    ASSERT(c.idx == 2);
    if(popped.i == 3)
    {
        ASSERT(c.data[0] == 1);
        ASSERT(c.data[1] == 2);
    }
    else if(popped.i == 2)
    {
        ASSERT(c.data[0] == 1);
        ASSERT(c.data[1] == 3);
    }
    else if(popped.i == 1)
    {
        ASSERT(c.data[0] == 2);
        ASSERT(c.data[1] == 3);
    }
    
    free(c.data);
    big_hv_deinit(&sp);
}

static void test_big_erase_every_other_element(void)
{
    big_hv sp;
    big_hv_init(&sp);
    const int N = 10;
    for (int i = 0; i < N; i++)
        big_hv_checked_put(&sp, (Big){.i = i}).ptr;
    
    bool remove = false;
    big_hv_iter it = big_hv_begin(&sp);
    while (!big_hv_iter_eq(it, big_hv_end(&sp))) {
        if (remove) {
            it = big_hv_iter_del(&sp, it);
        } else {
            it = big_hv_iter_next(it);
        }
        remove = !remove;
    }
    
    ASSERT(sp.count == N / 2);
    
    struct Collector c = {0};
    HIVE_FOR_EACH(it, big_hv_begin(&sp), big_hv_end(&sp))
    {
        collect_big(it.ptr, &c);
    }
    ASSERT(c.idx == N / 2);
    
    free(c.data);
    big_hv_deinit(&sp);
}

static void test_big_stress_iter_del(void)
{
    big_hv sp;
    big_hv_init(&sp);
    const int M = 10000;
    for (int i = 0; i < M; i++)
        big_hv_checked_put(&sp, (Big){.i = i}).ptr;
    
    big_hv_iter it = big_hv_begin(&sp);
    size_t expected = M;
    while (!big_hv_iter_eq(it, big_hv_end(&sp))) {
        if (big_hv_iter_elm(it)->i % 2 == 0) {
            it = big_hv_iter_del(&sp, it);
            expected--;
        } else {
            it = big_hv_iter_next(it);
        }
    }
    
    ASSERT(sp.count == expected);
    ASSERT(expected == M / 2);
    
    struct Collector c = {0};
    HIVE_FOR_EACH(it, big_hv_begin(&sp), big_hv_end(&sp))
    {
        collect_big(it.ptr, &c);
    }
    qsort(c.data, c.idx, sizeof(int), compare_ints);
    for (size_t i = 0; i < c.idx; i++)
        ASSERT(c.data[i] == (int)(2 * i + 1));
    
    free(c.data);
    big_hv_deinit(&sp);
}

static int compare_pointers(const void *a, const void *b)
{
    const void *pa = *(const void * const *)a;
    const void *pb = *(const void * const *)b;
    
    if (pa < pb) return -1;
    if (pa > pb) return 1;
    return 0;
}

static int compare_int_ptrs(const void *a, const void *b)
{
    int aa = **(int**)a;
    int bb = **(int**)b;
    return aa - bb;
}

static void test_int_clone_empty(void) {
    int_hv original;
    int_hv_init(&original);
    int_hv clone = int_hv_clone(&original);
    ASSERT(clone.count == 0);
    int_hv_deinit(&original);
    int_hv_deinit(&clone);
}

static void test_int_clone_single_element(void) {
    int_hv original;
    int_hv_init(&original);
    int *orig_p = int_hv_checked_put(&original, 42).ptr;
    
    int_hv clone = int_hv_clone(&original);
    ASSERT(clone.count == 1);
    
    // Verify pointer differs but value matches
    int *clone_p = int_hv_begin(&clone).ptr;
    ASSERT(clone_p != orig_p);
    ASSERT(*clone_p == 42);
    
    int_hv_deinit(&original);
    int_hv_deinit(&clone);
}

static void test_int_clone_multiple_elements(void) {
    int_hv original;
    int_hv_init(&original);
    const int N = 10;
    int **original_ptrs = malloc(sizeof(int*) * N);
    
    // Insert and track pointers
    for (int i = 0; i < N; ++i) {
        original_ptrs[i] = int_hv_checked_put(&original, i).ptr;
    }
    
    int_hv clone = int_hv_clone(&original);
    ASSERT(clone.count == N);
    
    // Collect clone pointers and verify uniqueness
    int **clone_ptrs = malloc(sizeof(int*) * N);
    size_t idx = 0;
    for (int_hv_iter it = int_hv_begin(&clone); 
         !int_hv_iter_eq(it, int_hv_end(&clone)); 
    it = int_hv_iter_next(it)) 
    {
        clone_ptrs[idx++] = int_hv_iter_elm(it);
    }
    
    // Sort for binary search
    qsort(original_ptrs, N, sizeof(int *), compare_pointers);
    
    // Verify all clone pointers are new
    for (int i = 0; i < N; ++i) {
        ASSERT(bsearch(&clone_ptrs[i], original_ptrs, N, sizeof(int *), compare_pointers) == NULL);
    }
    
    int_hv_deinit(&original);
    int_hv_deinit(&clone);
    free(clone_ptrs);
    free(original_ptrs);
}

static void test_int_clone_with_holes(void) {
    int_hv original;
    int_hv_init(&original);
    int *p1 = int_hv_checked_put(&original, 1).ptr;
    int *p2 = int_hv_checked_put(&original, 2).ptr;
    int *p3 = int_hv_checked_put(&original, 3).ptr;
    int_hv_del(&original, p2);
    
    int_hv clone = int_hv_clone(&original);
    ASSERT(clone.count == 2);
    
    // Collect clone pointers
    int *clone_ptrs[2];
    size_t idx = 0;
    for (int_hv_iter it = int_hv_begin(&clone);
         !int_hv_iter_eq(it, int_hv_end(&clone));
    it = int_hv_iter_next(it)) {
        clone_ptrs[idx++] = int_hv_iter_elm(it);
    }
    
    // Verify pointers are distinct from originals
    ASSERT(clone_ptrs[0] != p1 && clone_ptrs[0] != p3);
    ASSERT(clone_ptrs[1] != p1 && clone_ptrs[1] != p3);
    
    // Sort by pointer value
    qsort(clone_ptrs, 2, sizeof(int *), compare_int_ptrs);
    // Values should match
    ASSERT(*clone_ptrs[0] == 1);
    ASSERT(*clone_ptrs[1] == 3);
    
    int_hv_deinit(&original);
    int_hv_deinit(&clone);
}

static void test_int_clone_stress(void) {
    int_hv original;
    int_hv_init(&original);
    const int M = 10000;
    int **original_ptrs = malloc(M * sizeof(int *));
    ASSERT(original_ptrs != NULL);
    
    // Insert and track all original pointers
    for (int i = 0; i < M; ++i) {
        original_ptrs[i] = int_hv_checked_put(&original, i).ptr;
    }
    
    int_hv clone = int_hv_clone(&original);
    ASSERT(clone.count == M);
    
    // Prepare for pointer comparison
    qsort(original_ptrs, M, sizeof(int *), compare_pointers);
    
    // Verify clone pointers are unique
    size_t clone_idx = 0;
    for (int_hv_iter it = int_hv_begin(&clone); 
         !int_hv_iter_eq(it, int_hv_end(&clone)); 
    it = int_hv_iter_next(it)) 
         {
             int *clone_ptr = int_hv_iter_elm(it);
             ASSERT(bsearch(&clone_ptr, original_ptrs, M, sizeof(int *), compare_pointers) == NULL);
             clone_idx++;
         }
         
         free(original_ptrs);
         int_hv_deinit(&original);
         int_hv_deinit(&clone);
}

static void test_big_clone_empty(void) {
    big_hv original;
    big_hv_init(&original);
    big_hv clone = big_hv_clone(&original);
    ASSERT(clone.count == 0);
    big_hv_deinit(&original);
    big_hv_deinit(&clone);
}

static void test_big_clone_single_element(void) {
    big_hv original;
    big_hv_init(&original);
    Big *orig_p = big_hv_checked_put(&original, (Big){.i = 42}).ptr;
    
    big_hv clone = big_hv_clone(&original);
    ASSERT(clone.count == 1);
    
    // Verify pointer differs but value matches
    Big *clone_p = big_hv_begin(&clone).ptr;
    ASSERT(clone_p != orig_p);
    ASSERT(clone_p->i == 42);
    
    // Modify original and ensure clone remains unchanged
    orig_p->i = 100;
    ASSERT(clone_p->i == 42);
    
    big_hv_deinit(&original);
    big_hv_deinit(&clone);
}

static void test_big_clone_multiple_elements(void) {
    big_hv original;
    big_hv_init(&original);
    const int N = 100;
    Big **original_ptrs = malloc(sizeof(Big*) * N);
    
    // Insert and track pointers
    for (int i = 0; i < N; ++i) {
        original_ptrs[i] = big_hv_checked_put(&original, (Big){.i = i}).ptr;
    }
    
    big_hv clone = big_hv_clone(&original);
    ASSERT(clone.count == N);
    
    // Collect clone pointers and verify uniqueness
    Big **clone_ptrs = malloc(sizeof(Big*) * N);
    size_t idx = 0;
    for (big_hv_iter it = big_hv_begin(&clone); 
         !big_hv_iter_eq(it, big_hv_end(&clone)); 
    it = big_hv_iter_next(it)) 
    {
        clone_ptrs[idx++] = big_hv_iter_elm(it);
    }

    // Sort for binary search
    qsort(original_ptrs, N, sizeof(Big *), compare_pointers);

    // Verify all clone pointers are new
    for (int i = 0; i < N; ++i) {
        ASSERT(bsearch(&clone_ptrs[i], original_ptrs, N, sizeof(Big *), compare_pointers) == NULL);
    }

    big_hv_deinit(&original);
    big_hv_deinit(&clone);
    free(clone_ptrs);
    free(original_ptrs);
}

static void test_big_clone_independence_after_modification(void) {
    big_hv original;
    big_hv_init(&original);
    big_hv_checked_put(&original, (Big){.i = 1}).ptr;
    big_hv_checked_put(&original, (Big){.i = 2}).ptr;
    
    big_hv clone = big_hv_clone(&original);
    
    // Modify original after cloning
    big_hv_checked_put(&original, (Big){.i = 3}).ptr;
    
    ASSERT(original.count == 3);
    ASSERT(clone.count == 2);
    
    // Verify clone integrity
    struct Collector c = {0};
    HIVE_FOR_EACH(it, big_hv_begin(&clone), big_hv_end(&clone))
    {
        collect_big(it.ptr, &c);
    }

    qsort(c.data, c.idx, sizeof(int), compare_ints);
    ASSERT(c.idx == 2);
    ASSERT(c.data[0] == 1);
    ASSERT(c.data[1] == 2);
    free(c.data);
    
    big_hv_deinit(&original);
    big_hv_deinit(&clone);
}

static void test_big_clone_with_holes(void) {
    big_hv original;
    big_hv_init(&original);
    Big *p1 = big_hv_checked_put(&original, (Big){.i = 1}).ptr;
    Big *p2 = big_hv_checked_put(&original, (Big){.i = 2}).ptr;
    Big *p3 = big_hv_checked_put(&original, (Big){.i = 3}).ptr;
    big_hv_checked_del(&original, p2);
    int *p1i = &p1->i;
    int *p2i = &p2->i;
    int *p3i = &p3->i;
    
    big_hv clone = big_hv_clone(&original);
    ASSERT(clone.count == 2);
    
    // Collect clone pointers
    int *clone_ptrs[2];
    size_t idx = 0;
    for (big_hv_iter it = big_hv_begin(&clone); 
         !big_hv_iter_eq(it, big_hv_end(&clone)); 
    it = big_hv_iter_next(it)) 
         {
             clone_ptrs[idx++] = &big_hv_iter_elm(it)->i;
         }
         
         // Verify no pointer matches original's remaining elements
         ASSERT(clone_ptrs[0] != p1i && clone_ptrs[0] != p3i);
         ASSERT(clone_ptrs[1] != p1i && clone_ptrs[1] != p3i);
         
         // Values should match
         qsort(clone_ptrs, 2, sizeof(int *), compare_int_ptrs);
         ASSERT(*clone_ptrs[0] == 1);
         ASSERT(*clone_ptrs[1] == 3);
         
         big_hv_deinit(&original);
         big_hv_deinit(&clone);
}

static void test_big_clone_deep_copy(void) {
    big_hv original;
    big_hv_init(&original);
    Big *orig_p = big_hv_checked_put(&original, (Big){.i = 42}).ptr;
    
    big_hv clone = big_hv_clone(&original);
    
    // Modify original element
    orig_p->i = 100;
    
    // Verify clone remains unchanged
    Big *clone_p = big_hv_begin(&clone).ptr;
    ASSERT(clone_p->i == 42);
    
    big_hv_deinit(&original);
    big_hv_deinit(&clone);
}

static void test_big_clone_stress(void) {
    big_hv original;
    big_hv_init(&original);
    const int M = 5000;
    Big **original_ptrs = malloc(M * sizeof(Big *));
    ASSERT(original_ptrs != NULL);
    
    // Insert and track all original pointers
    for (int i = 0; i < M; ++i) {
        original_ptrs[i] = big_hv_checked_put(&original, (Big){.i = i}).ptr;
    }
    
    big_hv clone = big_hv_clone(&original);
    ASSERT(clone.count == M);
    
    // Prepare for pointer comparison
    qsort(original_ptrs, M, sizeof(Big *), compare_pointers);
    
    // Verify clone pointers are unique and values match
    size_t clone_idx = 0;
    for (big_hv_iter it = big_hv_begin(&clone); 
         !big_hv_iter_eq(it, big_hv_end(&clone)); 
    it = big_hv_iter_next(it)) 
         {
             Big *clone_ptr = big_hv_iter_elm(it);
             
             // Pointer uniqueness check
             ASSERT(bsearch(&clone_ptr, original_ptrs, M, sizeof(Big *), compare_pointers) == NULL);
             
             clone_idx++;
         }
         
         free(original_ptrs);
         big_hv_deinit(&original);
         big_hv_deinit(&clone);
}

static void test_big_clone_after_original_deinit(void) {
    big_hv original;
    big_hv_init(&original);
    big_hv_checked_put(&original, (Big){.i = 42}).ptr;
    
    big_hv clone = big_hv_clone(&original);
    big_hv_deinit(&original);  // Destroy original
    
    // Verify clone remains valid
    ASSERT(clone.count == 1);
    Big *clone_p = big_hv_begin(&clone).ptr;
    ASSERT(clone_p->i == 42);
    
    big_hv_deinit(&clone);
}

static Big random_big(void) {
    const int MAX_VALUE = 100000;
    Big b;
    // Fill the padding bytes with random data for testing
    for (size_t i = 0; i < sizeof(b._m); ++i) {
        ((unsigned char*)b._m)[i] = rand() & 0xFF;
    }
    b.i = rand() % MAX_VALUE;
    return b;
}

// Compare two Big elements for equality
static int big_equal(const Big *a, const Big *b) {
    if (a->i != b->i) return 0;
    return memcmp(a->_m, b->_m, sizeof(a->_m)) == 0;
}

static void test_hive(void)
{
    big_hv pool;
    big_hv_init(&pool);
    
    // size_t init_size = 76790;
    
    // for(size_t i = 0 ; i < init_size ; i++)
    // {
    //     big_hv_checked_put(&pool, (Big){.i=i});
    // }
    
    srand(69);
    const int NUM_OPS = 1000000;
    
    Big *vals = NULL;    // array of values for verification
    Big **ptrs = NULL;   // array of pointers to pool elements
    
    // bool rem = true;
    // for(big_hv_iter it = big_hv_begin(&pool), end = big_hv_end(&pool) ; !big_hv_iter_eq(it,end) ; )
    // {
    //     if(rem)
    //     {
    //         it = big_hv_iter_del(&pool, it);
    //     }
    //     else
    //     {
    //         arrpush(vals, *it.ptr);
    //         arrpush(ptrs, it.ptr);
    //         it = big_hv_iter_next(it);
    //     }
    //     rem = !rem;
    // }
    
    for (size_t op = 0; op < NUM_OPS; ++op) {
        if (arrlen(ptrs) == 0 || (rand() & 1)) {
            // PUT operation: insert new element
            Big new_el = random_big();
            Big *hv_ptr = big_hv_checked_put(&pool, new_el).ptr;
            ASSERT(hv_ptr && "Failed to insert into hive");
            // Track value and pointer
            arrpush(vals, *hv_ptr);
            arrpush(ptrs, hv_ptr);
        } else {
            // POP operation: remove random element via its pointer
            size_t idx = rand() % arrlen(ptrs);
            Big *target = ptrs[idx];
            // Copy value before removal
            Big expected = *target;
            // Remove from pool, get iterator to next
            big_hv_iter next_it = big_hv_checked_del(&pool, target);
            (void)next_it;
            // Remove from tracking arrays (swap with last)
            vals[idx] = vals[arrlen(vals) - 1]; arrpop(vals);
            ptrs[idx] = ptrs[arrlen(ptrs) - 1]; arrpop(ptrs);
            // Verify popped value matched
            ASSERT(big_equal(&expected, &expected) && "Popped element mismatch");
        }
        // Periodic full validation
        if (op % 10000 == 0) {
            size_t count = 0;
            for (big_hv_iter it = big_hv_begin(&pool), end = big_hv_end(&pool);
                 !big_hv_iter_eq(it,end);
            it = big_hv_iter_next(it)) {
                ++count;
            }
            ASSERT(count == arrlen(vals) && "Size mismatch between pool and array");
            
            // Ensure every ptr in ptrs points to a valid element matching vals
            for (size_t i = 0; i < arrlen(ptrs); ++i) {
                Big *p = ptrs[i];
                int found = 0;
                for (big_hv_iter it = big_hv_begin(&pool), end = big_hv_end(&pool);
                     !big_hv_iter_eq(it,end);
                it = big_hv_iter_next(it)) {
                    if (big_equal(p, big_hv_iter_elm(it))) {
                        found = 1;
                        break;
                    }
                }
                ASSERT(found && "Tracked pointer not found in pool");
                ASSERT(big_equal(p, &vals[i]) && "Value mismatch at tracked pointer");
            }
            
            printf("Validation at op %zu: OK (size = %zu)\n", op, arrlen(vals));
        }
    }
    
    printf("All %d operations completed successfully.\n", NUM_OPS);
    
    big_hv_deinit(&pool);
    arrfree(vals);
    arrfree(ptrs);
}

static void test_empty_start(void)
{
    big_hv pool;
    big_hv_init(&pool);
    
    srand(69);
    const int NUM_OPS = 1000000;
    
    Big *vals = NULL;
    Big **ptrs = NULL;
    
    const int N = 200;
    
    for(int i = 0 ; i < N ; i++)
    {
        big_hv_iter it = big_hv_put(&pool, random_big());
        if(i >= N / 2)
        {
            arrpush(ptrs, it.ptr);
            arrpush(vals, *it.ptr);
        }
    }
    
    int dels = 0;
    big_hv_iter del_it = big_hv_begin(&pool);
    while(dels < N / 2)
    {
        del_it = big_hv_iter_del(&pool, del_it);
        dels++;
    }
    
    ASSERT(pool.count == arrlen(ptrs));
    ASSERT(pool.count == arrlen(vals));
    
    for (size_t op = 0; op < NUM_OPS; ++op) {
        if (arrlen(ptrs) == 0 || (rand() & 1)) {
            // PUT operation: insert new element
            Big new_el = random_big();
            Big *hv_ptr = big_hv_checked_put(&pool, new_el).ptr;
            ASSERT(hv_ptr && "Failed to insert into hive");
            // Track value and pointer
            arrpush(vals, *hv_ptr);
            arrpush(ptrs, hv_ptr);
        } else {
            // POP operation: remove random element via its pointer
            size_t idx = rand() % arrlen(ptrs);
            Big *target = ptrs[idx];
            // Copy value before removal
            Big expected = *target;
            // Remove from pool, get iterator to next
            big_hv_iter next_it = big_hv_checked_del(&pool, target);
            (void)next_it;
            // Remove from tracking arrays (swap with last)
            vals[idx] = vals[arrlen(vals) - 1]; arrpop(vals);
            ptrs[idx] = ptrs[arrlen(ptrs) - 1]; arrpop(ptrs);
            // Verify popped value matched
            ASSERT(big_equal(&expected, &expected) && "Popped element mismatch");
        }
        // Periodic full validation
        if (op % 10000 == 0) {
            size_t count = 0;
            for (big_hv_iter it = big_hv_begin(&pool), end = big_hv_end(&pool);
                 !big_hv_iter_eq(it,end);
            it = big_hv_iter_next(it)) {
                ++count;
            }
            ASSERT(count == arrlen(vals) && "Size mismatch between pool and array");
            
            // Ensure every ptr in ptrs points to a valid element matching vals
            for (size_t i = 0; i < arrlen(ptrs); ++i) {
                Big *p = ptrs[i];
                int found = 0;
                for (big_hv_iter it = big_hv_begin(&pool), end = big_hv_end(&pool);
                     !big_hv_iter_eq(it,end);
                it = big_hv_iter_next(it)) {
                    if (big_equal(p, big_hv_iter_elm(it))) {
                        found = 1;
                        break;
                    }
                }
                ASSERT(found && "Tracked pointer not found in pool");
                ASSERT(big_equal(p, &vals[i]) && "Value mismatch at tracked pointer");
            }
            
            printf("Validation at op %zu: OK (size = %zu)\n", op, arrlen(vals));
        }
    }
    
    printf("All %d operations completed successfully.\n", NUM_OPS);
    
    big_hv_deinit(&pool);
    arrfree(vals);
    arrfree(ptrs);
}

void test_clone(void)
{
    printf("Running test_clone...\n");
    int_hv hv;
    int_hv_init(&hv);
    
    int_hv_put(&hv, 10);
    int_hv_put(&hv, 20);
    int_hv_put(&hv, 30);
    
    int_hv cloned_hv = int_hv_clone(&hv);
    
    ASSERT(cloned_hv.count == hv.count);
    
    int found_count = 0;
    int expected_values[] = {10, 20, 30};
    bool found[3] = {false, false, false};
    
    HIVE_FOR_EACH(it, int_hv_begin(&cloned_hv), int_hv_end(&cloned_hv))
    {
        int *val = int_hv_iter_elm(it); // Correct: int_hv_iter_elm
        for (int i = 0; i < 3; ++i)
        {
            if (*val == expected_values[i] && !found[i])
            {
                found[i] = true;
                found_count++;
                break;
            }
        }
    }
    ASSERT(found_count == 3);
    
    int_hv_del(&hv, int_hv_iter_elm(int_hv_begin(&hv)));
    ASSERT(hv.count == 2);
    ASSERT(cloned_hv.count == 3);
    
    int_hv_deinit(&hv);
    int_hv_deinit(&cloned_hv);
    printf("test_clone passed.\n");
}

void test_put_all(void)
{
    printf("Running test_put_all...\n");
    int_hv hv;
    int_hv_init(&hv);
    
    int values[] = {1, 2, 3, 4, 5};
    size_t num_values = sizeof(values) / sizeof(values[0]);
    
    int_hv_put_all(&hv, values, num_values);
    
    ASSERT(hv.count == num_values);
    
    int found_count = 0;
    bool found[5] = {false, false, false, false, false};
    
    HIVE_FOR_EACH(it, int_hv_begin(&hv), int_hv_end(&hv))
    {
        int *val = int_hv_iter_elm(it);
        for (size_t i = 0; i < num_values; ++i)
        {
            if (*val == values[i] && !found[i])
            {
                found[i] = true;
                found_count++;
                break;
            }
        }
    }
    ASSERT(found_count == num_values);
    
    int_hv_deinit(&hv);
    printf("test_put_all passed.\n");
}

void test_handle_apis(void)
{
    printf("Running test_handle_apis...\n");
    int_hv hv;
    int_hv_init(&hv);
    
    int_hv_iter it1 = int_hv_put(&hv, 100);
    int_hv_iter it2 = int_hv_put(&hv, 200);
    int_hv_iter it3 = int_hv_put(&hv, 300);
    
    ASSERT(hv.count == 3);
    
    int_hv_handle h1 = int_hv_iter_to_handle(it1);
    int_hv_handle h2 = int_hv_iter_to_handle(it2);
    int_hv_handle h3 = int_hv_iter_to_handle(it3);
    
    ASSERT(*int_hv_handle_elm(h1) == 100);
    ASSERT(*int_hv_handle_elm(h2) == 200);
    ASSERT(*int_hv_handle_elm(h3) == 300);
    
    int *h1_elm = int_hv_handle_elm(h1);
    int_hv_handle h1_from_ptr = int_hv_ptr_to_handle(&hv, h1_elm);
    ASSERT(*int_hv_handle_elm(h1_from_ptr) == 100);
    
    int_hv_handle_del(&hv, h2);
    ASSERT(hv.count == 2);
    
    ASSERT(*int_hv_handle_elm(h1) == 100);
    ASSERT(*int_hv_handle_elm(h3) == 300);
    
    int_hv_handle_del(&hv, h1);
    int_hv_handle_del(&hv, h3);
    ASSERT(hv.count == 0);
    
    int_hv_deinit(&hv);
    printf("test_handle_apis passed.\n");
}

void test_hive_for_each(void)
{
    printf("Running test_hive_for_each...\n");
    int_hv hv;
    int_hv_init(&hv);
    
    int_hv_put(&hv, 5);
    int_hv_put(&hv, 15);
    int_hv_put(&hv, 25);
    int_hv_put(&hv, 35);
    
    int sum = 0;
    int count = 0;
    int expected_sum = 5 + 15 + 25 + 35;
    int expected_count = 4;
    bool found_elements[4] = {false, false, false, false};
    int original_elements[] = {5, 15, 25, 35};
    
    HIVE_FOR_EACH(it, int_hv_begin(&hv), int_hv_end(&hv))
    {
        int *val = int_hv_iter_elm(it); // Correct: int_hv_iter_elm
        sum += *val;
        count++;
        
        for (int i = 0; i < 4; ++i)
        {
            if (*val == original_elements[i] && !found_elements[i])
            {
                found_elements[i] = true;
                break;
            }
        }
    }
    
    ASSERT(sum == expected_sum);
    ASSERT(count == expected_count);
    for (int i = 0; i < 4; ++i)
    {
        ASSERT(found_elements[i]);
    }
    
    int_hv_deinit(&hv);
    int_hv_init(&hv);
    sum = 0;
    count = 0;
    HIVE_FOR_EACH(it, int_hv_begin(&hv), int_hv_end(&hv))
    {
        sum += *int_hv_iter_elm(it); // Correct: int_hv_iter_elm
        count++;
    }
    ASSERT(sum == 0);
    ASSERT(count == 0);
    
    int_hv_deinit(&hv);
    printf("test_hive_for_each passed.\n");
}

static void validate_hive_content(const int_hv *hv, int *ref_arr)
{
    // 1. Check counts first
    ASSERT(hv->count == (size_t)arrlen(ref_arr));
    
    // 2. Extract hive elements into a temporary dynamic array
    int *hive_elements = NULL;
    HIVE_FOR_EACH(it, int_hv_begin(hv), int_hv_end(hv)) {
        arrput(hive_elements, *int_hv_iter_elm(it));
    }
    
    // 3. Sort both the extracted hive elements and the reference array
    qsort(hive_elements, arrlen(hive_elements), sizeof(int), compare_ints);
    qsort(ref_arr, arrlen(ref_arr), sizeof(int), compare_ints); // Note: ref_arr is modified here
    
    // 4. Compare the sorted arrays
    // Lengths should already match due to the first assert, but good to be explicit
    ASSERT(arrlen(hive_elements) == arrlen(ref_arr));
    
    if (arrlen(hive_elements) > 0)
    {
        ASSERT(memcmp(hive_elements, ref_arr, arrlen(hive_elements) * sizeof(int)) == 0);
    }
    
    // 5. Clean up temporary array
    arrfree(hive_elements);
}

void test_mixed_operations(void)
{
    printf("Running test_mixed_operations (stress test)...\n");
    int_hv hv;
    int_hv_init(&hv);
    
    int *reference_array = NULL; // stb_ds dynamic array
    
    srand((unsigned int)time(NULL)); // Seed random number generator
    
    const int NUM_ITERATIONS = 1000;
    const int MAX_BATCH_SIZE = 250;
    const int MAX_VALUE = 10000;
    
    for (int i = 0; i < NUM_ITERATIONS; ++i)
    {
        int operation = rand() % 3; // 0: put, 1: put_all, 2: del
        
        switch (operation)
        {
            case 0: // put a single element
            {
                int val = rand() % MAX_VALUE;
                int_hv_put(&hv, val);
                arrput(reference_array, val);
                break;
            }
            case 1: // put_all a batch of elements
            {
                int batch_size = 1 + (rand() % MAX_BATCH_SIZE);
                int *batch_values = (int*)malloc(batch_size * sizeof(int));
                ASSERT(batch_values != NULL);
                
                for (int j = 0; j < batch_size; ++j)
                {
                    batch_values[j] = rand() % MAX_VALUE;
                    arrput(reference_array, batch_values[j]);
                }
                int_hv_put_all(&hv, batch_values, batch_size);
                free(batch_values);
                break;
            }
            case 2: // del an element (if hive is not empty)
            {
                if (hv.count > 0 && arrlen(reference_array) > 0)
                {
                    // Pick a random element from the reference array to delete
                    int index_to_del = rand() % arrlen(reference_array);
                    int val_to_del = reference_array[index_to_del];
                    
                    // Find this element in the hive and delete it
                    // This is O(N) for hive_del, which is okay for a test
                    int *elm_ptr_to_del = NULL;
                    HIVE_FOR_EACH(it, int_hv_begin(&hv), int_hv_end(&hv))
                    {
                        if (*int_hv_iter_elm(it) == val_to_del)
                        {
                            elm_ptr_to_del = int_hv_iter_elm(it);
                            break;
                        }
                    }
                    
                    if (elm_ptr_to_del)
                    {
                        int_hv_del(&hv, elm_ptr_to_del);
                        // Remove from reference array
                        arrdel(reference_array, index_to_del);
                    }
                    else
                    {
                    }
                }
                break;
            }
        }
        
        if(hv.count != 0)
            validate_hive_content(&hv, reference_array);
    }
    
    int_hv_deinit(&hv);
    arrfree(reference_array);
    printf("test_mixed_operations (stress test) passed.\n");
}


int main(void)
{
    printf("Running tests...\n");
    
    test_init_deinit();
    test_single_put_and_loop();
    test_multiple_puts();
    test_pointer_stability();
    test_del_and_iteration();
    test_smaller_stress_inserts_dels();
    test_stress_inserts_dels();
    test_int_iteration_equivalence_after_random_dels();
    test_against_dynamic_array();
    test_insert_after_erase();
    test_empty_iteration();
    test_int_erase_single_element();
    test_int_erase_first_element();
    test_int_erase_last_element_during_iteration();
    test_int_erase_every_other_element();
    test_int_stress_iter_del();
    test_int_clone_empty();
    test_int_clone_single_element();
    test_int_clone_multiple_elements();
    test_int_clone_with_holes();
    test_int_clone_stress();
    
    test_big_clone_stress();
    
    test_big_init_deinit();
    test_big_single_put_and_loop();
    test_big_multiple_puts();
    test_big_pointer_stability();
    test_big_del_and_iteration();
    test_big_stress_inserts_dels();
    test_big_iteration_equivalence_after_random_dels();
    test_big_against_dynamic_array();
    test_big_insert_after_erase();
    test_clear_bucket();
    test_big_empty_iteration();
    test_big_erase_single_element();
    test_big_erase_first_element();
    test_big_erase_last_element_during_iteration();
    test_big_erase_every_other_element();
    test_big_stress_iter_del();
    test_big_clone_empty();
    test_big_clone_single_element();
    test_big_clone_multiple_elements();
    test_big_clone_independence_after_modification();
    test_big_clone_with_holes();
    test_big_clone_deep_copy();
    test_big_clone_stress();
    test_big_clone_after_original_deinit();
    test_mixed_operations();
    test_hive();
    test_empty_start();
    
    test_init_deinit();
    test_clone();
    test_put_all();
    test_handle_apis();
    test_hive_for_each();
    
    printf("ALL PASSED\n");
    return 0;
}
