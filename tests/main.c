#include <stdio.h>
#include <stdlib.h>
#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"

#define ASSERT(expr)                                                                                               \
do                                                                                                                 \
{                                                                                                                  \
    if (!(expr))                                                                                                   \
    {                                                                                                              \
        fprintf(stderr, "ASSERT FAILED: %s\n  at %s:%d in %s()\n", #expr, __FILE__, __LINE__, __func__);           \
        exit(1);                                                                                                   \
    }                                                                                                              \
} while (0)

static int compare_ints(const void *a, const void *b)
{
    int arg1 = *(const int *)a;
    int arg2 = *(const int *)b;
    return (arg1 > arg2) - (arg1 < arg2);
}

#define SP_IMPL
#define SP_TYPE int
#define SP_NAME int_sp
#include "stable_pool.h"

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
    int_sp sp;
    int_sp_init(&sp);
    ASSERT(sp.count == 0);
    int_sp_deinit(&sp);
}

static void test_single_put_and_loop(void)
{
    int_sp sp;
    int_sp_init(&sp);
    int *p = int_sp_put(&sp, 42);
    ASSERT(p && *p == 42);
    ASSERT(sp.count == (size_t)1);
    struct Collector c = {NULL, 0, 0};
    int_sp_foreach(&sp, collect_int, &c);
    ASSERT(c.idx == 1);
    ASSERT(c.data[0] == 42);
    free(c.data);
    int_sp_deinit(&sp);
}

static void test_multiple_puts(void)
{
    int_sp sp;
    int_sp_init(&sp);
    const int N = 100;
    for (int i = 0; i < N; i++)
        ASSERT(int_sp_put(&sp, i) != NULL);
    ASSERT(sp.count == (size_t)N);
    struct Collector c = {NULL, 0, 0};
    int_sp_foreach(&sp, collect_int, &c);
    ASSERT(c.idx == (size_t)N);
    
    qsort(c.data, c.idx, sizeof(int), compare_ints);
    for (size_t i = 0; i < c.idx; i++)
        ASSERT(c.data[i] == (int)i);
    free(c.data);
    int_sp_deinit(&sp);
}

static void test_pointer_stability(void)
{
    int_sp sp;
    int_sp_init(&sp);
    int *p1 = int_sp_put(&sp, 10);
    int *p2 = int_sp_put(&sp, 20);
    int *p3 = int_sp_put(&sp, 30);
    ASSERT(sp.count == 3);
    int_sp_pop(&sp, p3);
    ASSERT(sp.count == 2);
    ASSERT(*p1 == 10);
    ASSERT(*p2 == 20);
    int_sp_deinit(&sp);
}

static void test_pop_and_iteration(void)
{
    int_sp sp;
    int_sp_init(&sp);
    int *a = int_sp_put(&sp, 1);
    int *b = int_sp_put(&sp, 2);
    int *c = int_sp_put(&sp, 3);
    ASSERT(sp.count == 3);
    int_sp_pop(&sp, b);
    ASSERT(sp.count == 2);
    struct Collector col = {NULL, 0, 0};
    int_sp_foreach(&sp, collect_int, &col);
    ASSERT(col.idx == 2);
    
    qsort(col.data, col.idx, sizeof(int), compare_ints);
    ASSERT(col.data[0] == 1);
    ASSERT(col.data[1] == 3);
    
    free(col.data);
    int_sp_deinit(&sp);
    (void)a, (void)b, (void)c;
}

static void test_pop_invalid_pointer(void)
{
    int_sp sp;
    int_sp_init(&sp);
    int dummy = 0;
    ASSERT(sp.count == 0);
    int_sp_pop(&sp, &dummy);
    ASSERT(sp.count == 0);
    int_sp_deinit(&sp);
}

static void test_stress_inserts_pops(void)
{
    int_sp sp;
    int_sp_init(&sp);
    const int M = 10000;
    int **ptrs = malloc(M * sizeof *ptrs);
    ASSERT(ptrs != NULL);
    for (int i = 0; i < M; i++)
        ptrs[i] = int_sp_put(&sp, i);
    ASSERT(sp.count == (size_t)M);
    
    for (int i = 0; i < M; i += 2)
        int_sp_pop(&sp, ptrs[i]);
    ASSERT(sp.count == (size_t)M / 2);
    struct Collector c = {NULL, 0, 0};
    int_sp_foreach(&sp, collect_int, &c);
    ASSERT(c.idx == (size_t)(M / 2));
    
    qsort(c.data, c.idx, sizeof(int), compare_ints);
    for (size_t i = 0; i < c.idx; i++)
        ASSERT(c.data[i] == 1 + 2 * (int)i);
    
    free(c.data);
    free(ptrs);
    int_sp_deinit(&sp);
}

static void test_smaller_stress_inserts_pops(void)
{
    int_sp sp;
    int_sp_init(&sp);
    const int M = 500;
    int **ptrs = malloc(M * sizeof *ptrs);
    ASSERT(ptrs != NULL);
    
    for (int i = 0; i < M; i++)
    {
        ptrs[i] = int_sp_put(&sp, i);
        ASSERT(ptrs[i] != NULL);
    }
    ASSERT(sp.count == (size_t)M);
    
    for (int i = 0; i < M; i += 2)
        int_sp_pop(&sp, ptrs[i]);
    ASSERT(sp.count == (size_t)M / 2);
    
    struct Collector c = {NULL, 0, 0};
    int_sp_foreach(&sp, collect_int, &c);
    ASSERT(c.idx == (size_t)(M / 2));
    
    qsort(c.data, c.idx, sizeof(int), compare_ints);
    for (size_t i = 0; i < c.idx; i++)
        ASSERT(c.data[i] == 1 + 2 * (int)i);
    
    free(c.data);
    free(ptrs);
    int_sp_deinit(&sp);
}

typedef struct Big
{
    char _m[256 - 4];
    int i;
} Big;

#define SP_IMPL
#define SP_TYPE Big
#define SP_NAME big_sp
#include "stable_pool.h"

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
    big_sp sp;
    big_sp_init(&sp);
    ASSERT(sp.count == 0);
    big_sp_deinit(&sp);
}

static void test_big_single_put_and_loop(void)
{
    big_sp sp;
    big_sp_init(&sp);
    Big *p = big_sp_put(&sp, (Big){.i = 42});
    ASSERT(p && p->i == 42);
    ASSERT(sp.count == 1);
    
    struct Collector c = {NULL, 0, 0};
    SP_FOREACH(&sp, collect_big(SP_IT, &c););
    ASSERT(c.idx == 1);
    ASSERT(c.data[0] == 42);
    free(c.data);
    big_sp_deinit(&sp);
}

static void test_big_multiple_puts(void)
{
    big_sp sp;
    big_sp_init(&sp);
    const int N = 100;
    for (int i = 0; i < N; i++)
    {
        Big *p = big_sp_put(&sp, (Big){.i = i});
        ASSERT(p != NULL);
    }
    ASSERT(sp.count == (size_t)N);
    
    struct Collector c = {NULL, 0, 0};
    for (big_sp_iter_t it = big_sp_begin(&sp), end = big_sp_end(&sp); !big_sp_iter_eq(it, end);
         it = big_sp_iter_next(it))
         {
             collect_big(big_sp_iter_elm(it), &c);
         }
         ASSERT(c.idx == (size_t)N);
    
    qsort(c.data, c.idx, sizeof(int), compare_ints);
    for (size_t i = 0; i < c.idx; i++)
        ASSERT(c.data[i] == (int)i);
    
    free(c.data);
    big_sp_deinit(&sp);
}

static void test_big_pointer_stability(void)
{
    big_sp sp;
    big_sp_init(&sp);
    Big *p1 = big_sp_put(&sp, (Big){.i = 10});
    Big *p2 = big_sp_put(&sp, (Big){.i = 20});
    Big *p3 = big_sp_put(&sp, (Big){.i = 30});
    ASSERT(sp.count == 3);
    big_sp_pop(&sp, p3);
    ASSERT(sp.count == 2);
    ASSERT(p1->i == 10);
    ASSERT(p2->i == 20);
    big_sp_deinit(&sp);
}

static void test_big_pop_and_iteration(void)
{
    big_sp sp;
    big_sp_init(&sp);
    Big *a = big_sp_put(&sp, (Big){.i = 1});
    Big *b = big_sp_put(&sp, (Big){.i = 2});
    Big *c = big_sp_put(&sp, (Big){.i = 3});
    ASSERT(sp.count == 3);
    big_sp_pop(&sp, b);
    ASSERT(sp.count == 2);
    
    struct Collector col = {NULL, 0, 0};
    SP_FOREACH(&sp, collect_big(SP_IT, &col););
    ASSERT(col.idx == 2);
    
    qsort(col.data, col.idx, sizeof(int), compare_ints);
    ASSERT(col.data[0] == 1);
    ASSERT(col.data[1] == 3);
    
    free(col.data);
    big_sp_deinit(&sp);
    
    (void)a, (void)b, (void)c;
}

static void test_big_pop_invalid_pointer(void)
{
    big_sp sp;
    big_sp_init(&sp);
    Big dummy = {.i = -1};
    ASSERT(sp.count == 0);
    big_sp_pop(&sp, &dummy);
    ASSERT(sp.count == 0);
    big_sp_deinit(&sp);
}

static void test_big_stress_inserts_pops(void)
{
    big_sp sp;
    big_sp_init(&sp);
    const int M = 2048 * 32;
    Big **ptrs = malloc(M * sizeof *ptrs);
    ASSERT(ptrs != NULL);
    
    for (int i = 0; i < M; i++)
    {
        ptrs[i] = big_sp_put(&sp, (Big){.i = i});
        ASSERT(ptrs[i] != NULL);
    }
    ASSERT(sp.count == (size_t)M);
    
    for (int i = 0; i < M; i += 2)
        big_sp_pop(&sp, ptrs[i]);
    ASSERT(sp.count == (size_t)M / 2);
    
    struct Collector col = {NULL, 0, 0};
    for (big_sp_iter_t it = big_sp_begin(&sp), end = big_sp_end(&sp); !big_sp_iter_eq(it, end);
         it = big_sp_iter_next(it))
         {
             collect_big(big_sp_iter_elm(it), &col);
         }
         ASSERT(col.idx == (size_t)M / 2);
    
    qsort(col.data, col.idx, sizeof(int), compare_ints);
    for (size_t i = 0; i < col.idx; i++)
        ASSERT(col.data[i] == 1 + 2 * (int)i);
    
    free(col.data);
    free(ptrs);
    big_sp_deinit(&sp);
}

static void test_int_iteration_equivalence_after_random_pops(void)
{
    int_sp sp;
    int_sp_init(&sp);
    const int N = 1024;
    int **ptrs = malloc(N * sizeof *ptrs);
    ASSERT(ptrs != NULL);
    
    srand(42);
    for (int i = 0; i < N; i++)
        ptrs[i] = int_sp_put(&sp, i);
    
    for (int i = 0; i < N; i++)
    {
        if (rand() % 2)
        {
            int_sp_pop(&sp, ptrs[i]);
            ptrs[i] = NULL;
        }
    }
    
    struct Collector col1 = {NULL, 0, 0};
    struct Collector col2 = {NULL, 0, 0};
    struct Collector col3 = {NULL, 0, 0};
    
    int_sp_foreach(&sp, collect_int, &col1);
    
    SP_FOREACH(&sp, collect_int(SP_IT, &col2););
    
    for (int_sp_iter_t it = int_sp_begin(&sp), end = int_sp_end(&sp); !int_sp_iter_eq(it, end);
         it = int_sp_iter_next(it))
         {
             collect_int(int_sp_iter_elm(it), &col3);
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
    int_sp_deinit(&sp);
}

static void test_big_iteration_equivalence_after_random_pops(void)
{
    big_sp sp;
    big_sp_init(&sp);
    const int N = 1024;
    Big **ptrs = malloc(N * sizeof *ptrs);
    ASSERT(ptrs != NULL);
    
    srand(1337);
    for (int i = 0; i < N; i++)
        ptrs[i] = big_sp_put(&sp, (Big){.i = i});
    
    for (int i = 0; i < N; i++)
    {
        if (rand() % 2)
        {
            big_sp_pop(&sp, ptrs[i]);
            ptrs[i] = NULL;
        }
    }
    
    struct Collector col1 = {NULL, 0, 0};
    struct Collector col2 = {NULL, 0, 0};
    struct Collector col3 = {NULL, 0, 0};
    
    big_sp_foreach(&sp, collect_big, &col1);
    SP_FOREACH(&sp, collect_big(SP_IT, &col2););
    for (big_sp_iter_t it = big_sp_begin(&sp), end = big_sp_end(&sp); !big_sp_iter_eq(it, end);
         it = big_sp_iter_next(it))
         {
             collect_big(big_sp_iter_elm(it), &col3);
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
    big_sp_deinit(&sp);
}

static void test_against_dynamic_array(void)
{
    int_sp sp;
    int_sp_init(&sp);
    int *expected = NULL;
    const int N = 20;
    for (int i = 0; i < N; i++)
    {
        int *ptr = int_sp_put(&sp, i);
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
        
        int_sp_iter_t it;
        int *found = NULL;
        for (it = int_sp_begin(&sp); !int_sp_iter_eq(it, int_sp_end(&sp)); it = int_sp_iter_next(it))
        {
            int *current = int_sp_iter_elm(it);
            if (*current == value)
            {
                found = current;
                break;
            }
        }
        ASSERT(found != NULL);
        
        int_sp_pop(&sp, found);
        
        arrdel(expected, idx);
    }
    
    struct Collector collector = {0};
    int_sp_foreach(&sp, collect_int, &collector);
    
    qsort(collector.data, collector.idx, sizeof(int), compare_ints);
    qsort(expected, arrlen(expected), sizeof(int), compare_ints);
    
    ASSERT(collector.idx == (size_t)arrlen(expected));
    
    for (size_t i = 0; i < collector.idx; i++)
    {
        ASSERT(collector.data[i] == expected[i]);
    }
    
    free(collector.data);
    arrfree(expected);
    int_sp_deinit(&sp);
}

static void test_big_against_dynamic_array(void)
{
    big_sp sp;
    big_sp_init(&sp);
    int *expected = NULL;
    const int N = 50;
    for (int i = 0; i < N; i++)
    {
        Big *ptr = big_sp_put(&sp, (Big){.i = i});
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
        
        big_sp_iter_t it;
        Big *found = NULL;
        for (it = big_sp_begin(&sp); !big_sp_iter_eq(it, big_sp_end(&sp)); it = big_sp_iter_next(it))
        {
            Big *current = big_sp_iter_elm(it);
            if (current->i == value)
            {
                found = current;
                break;
            }
        }
        ASSERT(found != NULL);
        
        big_sp_pop(&sp, found);
        arrdel(expected, idx);
    }
    
    struct Collector collector = {0};
    big_sp_foreach(&sp, collect_big, &collector);
    
    qsort(collector.data, collector.idx, sizeof(int), compare_ints);
    qsort(expected, arrlen(expected), sizeof(int), compare_ints);
    
    ASSERT(collector.idx == (size_t)arrlen(expected));
    for (size_t i = 0; i < collector.idx; i++)
    {
        ASSERT(collector.data[i] == expected[i]);
    }
    
    free(collector.data);
    arrfree(expected);
    big_sp_deinit(&sp);
}

static void test_insert_after_erase(void)
{
    int_sp sp;
    int_sp_init(&sp);
    int *expected = NULL;
    
    const int N = 1000;
    for (int i = 0; i < N; i++)
    {
        int *ptr = int_sp_put(&sp, i);
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
        
        int_sp_iter_t it;
        int *found = NULL;
        for (it = int_sp_begin(&sp); !int_sp_iter_eq(it, int_sp_end(&sp)); it = int_sp_iter_next(it))
        {
            int *current = int_sp_iter_elm(it);
            if (*current == value)
            {
                found = current;
                break;
            }
        }
        ASSERT(found != NULL);
        
        int_sp_pop(&sp, found);
        arrdel(expected, idx);
    }
    
    const int M = 2000;
    for (int i = N; i < N + M; i++)
    {
        int *ptr = int_sp_put(&sp, i);
        ASSERT(ptr != NULL);
        arrput(expected, i);
    }
    ASSERT(sp.count == (size_t)arrlen(expected));
    
    struct Collector collector = {0};
    int_sp_foreach(&sp, collect_int, &collector);
    
    qsort(collector.data, collector.idx, sizeof(int), compare_ints);
    qsort(expected, arrlen(expected), sizeof(int), compare_ints);
    
    ASSERT(collector.idx == (size_t)arrlen(expected));
    for (size_t i = 0; i < collector.idx; i++)
    {
        ASSERT(collector.data[i] == expected[i]);
    }
    
    free(collector.data);
    arrfree(expected);
    int_sp_deinit(&sp);
}

static void test_big_insert_after_erase(void)
{
    big_sp sp;
    big_sp_init(&sp);
    int *expected = NULL;
    
    const int N = 500;
    for (int i = 0; i < N; i++)
    {
        Big *ptr = big_sp_put(&sp, (Big){.i = i});
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
        
        big_sp_iter_t it;
        Big *found = NULL;
        for (it = big_sp_begin(&sp); !big_sp_iter_eq(it, big_sp_end(&sp)); it = big_sp_iter_next(it))
        {
            Big *current = big_sp_iter_elm(it);
            if (current->i == value)
            {
                found = current;
                break;
            }
        }
        ASSERT(found != NULL);
        
        big_sp_pop(&sp, found);
        arrdel(expected, idx);
    }
    
    const int M = 1000;
    for (int i = N; i < N + M; i++)
    {
        Big *ptr = big_sp_put(&sp, (Big){.i = i});
        ASSERT(ptr != NULL);
        arrput(expected, i);
    }
    ASSERT(sp.count == (size_t)arrlen(expected));
    
    struct Collector collector = {0};
    big_sp_foreach(&sp, collect_big, &collector);
    
    qsort(collector.data, collector.idx, sizeof(int), compare_ints);
    qsort(expected, arrlen(expected), sizeof(int), compare_ints);
    
    ASSERT(collector.idx == (size_t)arrlen(expected));
    for (size_t i = 0; i < collector.idx; i++)
    {
        ASSERT(collector.data[i] == expected[i]);
    }
    
    free(collector.data);
    arrfree(expected);
    big_sp_deinit(&sp);
}

void test_clear_bucket()
{
    big_sp sp;
    big_sp_init(&sp);
    
    Big **to_delete = NULL;
    Big *copy = NULL;
    for(int i = 0 ; i < 512 * 3 ; i++)
    {
        arrput(to_delete, big_sp_put(&sp, (Big){.i=i} ));
        arrput(copy, *to_delete[i]);
    }
    
    for(int i = 512 ; i < 512 * 2 ; i++)
    {
        big_sp_pop(&sp, to_delete[i]);
        arrdel(copy, i);
    }
    
    printf("NB BUCKETS = %zu\n", sp.bucket_count);
    
    int i = 0;
    SP_FOREACH(&sp,
    {
        
    });
    
    big_sp_deinit(&sp);
}

int main(void)
{
    printf("Running tests...\n");
    test_init_deinit();
    test_single_put_and_loop();
    test_multiple_puts();
    test_pointer_stability();
    test_pop_and_iteration();
    test_pop_invalid_pointer();
    test_smaller_stress_inserts_pops();
    test_stress_inserts_pops();
    test_int_iteration_equivalence_after_random_pops();
    test_against_dynamic_array();
    test_insert_after_erase();
    
    test_big_init_deinit();
    test_big_single_put_and_loop();
    test_big_multiple_puts();
    test_big_pointer_stability();
    test_big_pop_and_iteration();
    test_big_pop_invalid_pointer();
    test_big_stress_inserts_pops();
    test_big_iteration_equivalence_after_random_pops();
    test_big_against_dynamic_array();
    test_big_insert_after_erase();
    
    printf("ALL PASSED\n");
    return 0;
}
