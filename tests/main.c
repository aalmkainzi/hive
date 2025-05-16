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
    const int M = 200;
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
    const int N = 20;
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

        SP_FOREACH(&sp,
                   collect_int(SP_IT, &col2);
        );

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
    
    qsort(collector.data, collector.idx,    sizeof(int), compare_ints);
    qsort(expected,       arrlen(expected), sizeof(int), compare_ints);
    
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
    
    const int N = 250;
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
    
    const int M = 500;
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
    int bucket_size = SP_GET_BUCKET_SIZE(&sp);
    for(int i = 0 ; i < bucket_size + bucket_size + bucket_size ; i++)
    {
        arrput(to_delete, big_sp_put(&sp, (Big){.i=i} ));
        arrput(copy, (Big){.i=i});
    }
    
    printf("\nNB BUCKETS = %zu\n", sp.bucket_count);
    
    for(int i = bucket_size, j = bucket_size ; i < bucket_size + bucket_size ; i++)
    {
        big_sp_pop(&sp, to_delete[j]);
        arrdel(copy, j);
        arrdel(to_delete, j);
    }
    
    printf("NB BUCKETS = %zu\n\n", sp.bucket_count);
    
    int i = 0;
    
    SP_FOREACH(&sp,
               {
                   Big it = *SP_IT;
                   i++;
               }
    );
    
    big_sp_deinit(&sp);
    arrfree(to_delete);
    arrfree(copy);
}

static void test_empty_iteration(void)
{
    int_sp sp;
    int_sp_init(&sp);
    ASSERT(sp.count == 0);
    
    struct Collector c_foreach = {NULL, 0, 0};
    int_sp_foreach(&sp, collect_int, &c_foreach);
    ASSERT(c_foreach.idx == 0);
    
    struct Collector c_macro = {NULL, 0, 0};
    SP_FOREACH(&sp, collect_int(SP_IT, &c_macro););
    ASSERT(c_macro.idx == 0);
    
    struct Collector c_iter = {NULL, 0, 0};
    for (int_sp_iter_t it = int_sp_begin(&sp),
        end = int_sp_end(&sp); 
    !int_sp_iter_eq(it, end); 
    it = int_sp_iter_next(it))
    {
        collect_int(int_sp_iter_elm(it), &c_iter);
    }
    ASSERT(c_iter.idx == 0);
    
    int_sp_deinit(&sp);
}

static void test_big_empty_iteration(void)
{
    big_sp sp;
    big_sp_init(&sp);
    ASSERT(sp.count == 0);
    
    struct Collector c_foreach = {NULL, 0, 0};
    big_sp_foreach(&sp, collect_big, &c_foreach);
    ASSERT(c_foreach.idx == 0);
    
    struct Collector c_macro = {NULL, 0, 0};
    SP_FOREACH(&sp, collect_big(SP_IT, &c_macro););
    ASSERT(c_macro.idx == 0);
    
    struct Collector c_iter = {NULL, 0, 0};
    for (big_sp_iter_t it = big_sp_begin(&sp), end = big_sp_end(&sp); 
         !big_sp_iter_eq(it, end); 
    it = big_sp_iter_next(it))
         {
             collect_big(big_sp_iter_elm(it), &c_iter);
         }
         ASSERT(c_iter.idx == 0);
         
         big_sp_deinit(&sp);
}

static void test_int_erase_single_element(void)
{
    int_sp sp;
    int_sp_init(&sp);
    int *p = int_sp_put(&sp, 42);
    ASSERT(p != NULL);
    
    int_sp_iter_t it = int_sp_begin(&sp);
    it = int_sp_iter_pop(it);
    
    ASSERT(int_sp_iter_eq(it, int_sp_end(&sp)));
    ASSERT(sp.count == 0);
    int_sp_deinit(&sp);
}

static void test_int_erase_first_element(void)
{
    int_sp sp;
    int_sp_init(&sp);
    int_sp_put(&sp, 1);
    int_sp_put(&sp, 2);
    int_sp_put(&sp, 3);
    
    int_sp_iter_t it = int_sp_begin(&sp);
    int elm = *int_sp_iter_elm(it);
    it = int_sp_iter_pop(it);
    
    ASSERT(sp.count == 2);
    
    struct Collector c = {0};
    int_sp_foreach(&sp, collect_int, &c);
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
    int_sp_deinit(&sp);
}

static void test_int_erase_last_element_during_iteration(void)
{
    int_sp sp;
    int_sp_init(&sp);
    int_sp_put(&sp, 1);
    int_sp_put(&sp, 2);
    int_sp_put(&sp, 3);
    
    int_sp_iter_t it = int_sp_begin(&sp);
    int_sp_iter_t last = int_sp_end(&sp);
    while (!int_sp_iter_eq(it, int_sp_end(&sp))) {
        last = it;
        it = int_sp_iter_next(it);
    }
    it = last;
    int elm = *int_sp_iter_elm(it);
    it = int_sp_iter_pop(it);
    
    ASSERT(sp.count == 2);
    
    struct Collector c = {0};
    int_sp_foreach(&sp, collect_int, &c);
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
    int_sp_deinit(&sp);
}

static void test_int_erase_every_other_element(void)
{
    int_sp sp;
    int_sp_init(&sp);
    const int N = 10;
    for (int i = 0; i < N; i++)
        int_sp_put(&sp, i);
    
    bool remove = false;
    int_sp_iter_t it = int_sp_begin(&sp);
    while (!int_sp_iter_eq(it, int_sp_end(&sp))) {
        if (remove) {
            it = int_sp_iter_pop(it);
        } else {
            it = int_sp_iter_next(it);
        }
        remove = !remove;
    }
    
    ASSERT(sp.count == N / 2);
    
    struct Collector c = {0};
    int_sp_foreach(&sp, collect_int, &c);
    ASSERT(c.idx == N / 2);
    
    free(c.data);
    int_sp_deinit(&sp);
}

static void test_int_stress_iter_pop(void)
{
    int_sp sp;
    int_sp_init(&sp);
    const int M = 10000;
    for (int i = 0; i < M; i++)
        int_sp_put(&sp, i);
    
    int_sp_iter_t it = int_sp_begin(&sp);
    size_t expected = M;
    while (!int_sp_iter_eq(it, int_sp_end(&sp))) {
        if (*(int_sp_iter_elm(it)) % 2 == 0) {
            it = int_sp_iter_pop(it);
            expected--;
        } else {
            it = int_sp_iter_next(it);
        }
    }
    
    ASSERT(sp.count == expected);
    ASSERT(expected == M / 2);
    
    struct Collector c = {0};
    int_sp_foreach(&sp, collect_int, &c);
    qsort(c.data, c.idx, sizeof(int), compare_ints);
    for (size_t i = 0; i < c.idx; i++)
        ASSERT(c.data[i] == (int)(2 * i + 1));
    
    free(c.data);
    int_sp_deinit(&sp);
}


static void test_big_erase_single_element(void)
{
    big_sp sp;
    big_sp_init(&sp);
    Big *p = big_sp_put(&sp, (Big){.i = 42});
    ASSERT(p != NULL);
    
    big_sp_iter_t it = big_sp_begin(&sp);
    it = big_sp_iter_pop(it);
    
    ASSERT(big_sp_iter_eq(it, big_sp_end(&sp)));
    ASSERT(sp.count == 0);
    big_sp_deinit(&sp);
}

static void test_big_erase_first_element(void)
{
    big_sp sp;
    big_sp_init(&sp);
    big_sp_put(&sp, (Big){.i = 1});
    big_sp_put(&sp, (Big){.i = 2});
    big_sp_put(&sp, (Big){.i = 3});
    
    big_sp_iter_t it = big_sp_begin(&sp);
    Big popped = *big_sp_iter_elm(it);
    it = big_sp_iter_pop(it);
    
    ASSERT(sp.count == 2);
    
    struct Collector c = {0};
    big_sp_foreach(&sp, collect_big, &c);
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
    big_sp_deinit(&sp);
}

static void test_big_erase_last_element_during_iteration(void)
{
    big_sp sp;
    big_sp_init(&sp);
    big_sp_put(&sp, (Big){.i = 1});
    big_sp_put(&sp, (Big){.i = 2});
    big_sp_put(&sp, (Big){.i = 3});
    
    big_sp_iter_t it = big_sp_begin(&sp);
    big_sp_iter_t last = big_sp_end(&sp);
    while (!big_sp_iter_eq(it, big_sp_end(&sp))) {
        last = it;
        it = big_sp_iter_next(it);
    }
    it = last;
    Big popped = *big_sp_iter_elm(it);
    it = big_sp_iter_pop(it);
    
    ASSERT(sp.count == 2);
    
    struct Collector c = {0};
    big_sp_foreach(&sp, collect_big, &c);
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
    big_sp_deinit(&sp);
}

static void test_big_erase_every_other_element(void)
{
    big_sp sp;
    big_sp_init(&sp);
    const int N = 10;
    for (int i = 0; i < N; i++)
        big_sp_put(&sp, (Big){.i = i});
    
    bool remove = false;
    big_sp_iter_t it = big_sp_begin(&sp);
    while (!big_sp_iter_eq(it, big_sp_end(&sp))) {
        if (remove) {
            it = big_sp_iter_pop(it);
        } else {
            it = big_sp_iter_next(it);
        }
        remove = !remove;
    }
    
    ASSERT(sp.count == N / 2);
    
    struct Collector c = {0};
    big_sp_foreach(&sp, collect_big, &c);
    ASSERT(c.idx == N / 2);
    
    free(c.data);
    big_sp_deinit(&sp);
}

static void test_big_stress_iter_pop(void)
{
    big_sp sp;
    big_sp_init(&sp);
    const int M = 10000;
    for (int i = 0; i < M; i++)
        big_sp_put(&sp, (Big){.i = i});
    
    big_sp_iter_t it = big_sp_begin(&sp);
    size_t expected = M;
    while (!big_sp_iter_eq(it, big_sp_end(&sp))) {
        if (big_sp_iter_elm(it)->i % 2 == 0) {
            it = big_sp_iter_pop(it);
            expected--;
        } else {
            it = big_sp_iter_next(it);
        }
    }
    
    ASSERT(sp.count == expected);
    ASSERT(expected == M / 2);
    
    struct Collector c = {0};
    big_sp_foreach(&sp, collect_big, &c);
    qsort(c.data, c.idx, sizeof(int), compare_ints);
    for (size_t i = 0; i < c.idx; i++)
        ASSERT(c.data[i] == (int)(2 * i + 1));
    
    free(c.data);
    big_sp_deinit(&sp);
}

static int compare_pointers(const void *a, const void *b) {
    return (size_t)*(void**)a - (size_t)*(void**)b;
}

static int compare_int_ptrs(const void *a, const void *b)
{
    int aa = **(int**)a;
    int bb = **(int**)b;
    return aa - bb;
}

static void test_int_clone_empty(void) {
    int_sp original;
    int_sp_init(&original);
    int_sp clone = int_sp_clone(&original);
    ASSERT(clone.count == 0);
    int_sp_deinit(&original);
    int_sp_deinit(&clone);
}

static void test_int_clone_single_element(void) {
    int_sp original;
    int_sp_init(&original);
    int *orig_p = int_sp_put(&original, 42);
    
    int_sp clone = int_sp_clone(&original);
    ASSERT(clone.count == 1);
    
    // Verify pointer differs but value matches
    int *clone_p = &int_sp_begin(&clone).elm->value;
    ASSERT(clone_p != orig_p);
    ASSERT(*clone_p == 42);
    
    int_sp_deinit(&original);
    int_sp_deinit(&clone);
}

static void test_int_clone_multiple_elements(void) {
    int_sp original;
    int_sp_init(&original);
    constexpr int N = 10;
    int *original_ptrs[N];
    
    // Insert and track pointers
    for (int i = 0; i < N; ++i) {
        original_ptrs[i] = int_sp_put(&original, i);
    }
    
    int_sp clone = int_sp_clone(&original);
    ASSERT(clone.count == N);
    
    // Collect clone pointers and verify uniqueness
    int *clone_ptrs[N];
    size_t idx = 0;
    for (int_sp_iter_t it = int_sp_begin(&clone); 
         !int_sp_iter_eq(it, int_sp_end(&clone)); 
    it = int_sp_iter_next(it)) 
         {
             clone_ptrs[idx++] = int_sp_iter_elm(it);
         }
         
         // Sort for binary search
         qsort(original_ptrs, N, sizeof(int *), compare_pointers);
         
         // Verify all clone pointers are new
         for (int i = 0; i < N; ++i) {
             ASSERT(bsearch(&clone_ptrs[i], original_ptrs, N, sizeof(int *), compare_pointers) == NULL);
         }
         
         int_sp_deinit(&original);
         int_sp_deinit(&clone);
}

static void test_int_clone_with_holes(void) {
    int_sp original;
    int_sp_init(&original);
    int *p1 = int_sp_put(&original, 1);
    int *p2 = int_sp_put(&original, 2);
    int *p3 = int_sp_put(&original, 3);
    int_sp_pop(&original, p2);
    
    int_sp clone = int_sp_clone(&original);
    ASSERT(clone.count == 2);
    
    // Collect clone pointers
    int *clone_ptrs[2];
    size_t idx = 0;
    for (int_sp_iter_t it = int_sp_begin(&clone);
         !int_sp_iter_eq(it, int_sp_end(&clone));
    it = int_sp_iter_next(it)) {
        clone_ptrs[idx++] = int_sp_iter_elm(it);
    }
    
    // Verify pointers are distinct from originals
    ASSERT(clone_ptrs[0] != p1 && clone_ptrs[0] != p3);
    ASSERT(clone_ptrs[1] != p1 && clone_ptrs[1] != p3);
    
    // Sort by pointer value
    qsort(clone_ptrs, 2, sizeof(int *), compare_int_ptrs);
    // Values should match
    ASSERT(*clone_ptrs[0] == 1);
    ASSERT(*clone_ptrs[1] == 3);
    
    int_sp_deinit(&original);
    int_sp_deinit(&clone);
}

static void test_int_clone_stress(void) {
    int_sp original;
    int_sp_init(&original);
    const int M = 10000;
    int **original_ptrs = malloc(M * sizeof(int *));
    ASSERT(original_ptrs != NULL);
    
    // Insert and track all original pointers
    for (int i = 0; i < M; ++i) {
        original_ptrs[i] = int_sp_put(&original, i);
    }
    
    int_sp clone = int_sp_clone(&original);
    ASSERT(clone.count == M);
    
    // Prepare for pointer comparison
    qsort(original_ptrs, M, sizeof(int *), compare_pointers);
    
    // Verify clone pointers are unique
    size_t clone_idx = 0;
    for (int_sp_iter_t it = int_sp_begin(&clone); 
         !int_sp_iter_eq(it, int_sp_end(&clone)); 
    it = int_sp_iter_next(it)) 
         {
             int *clone_ptr = int_sp_iter_elm(it);
             ASSERT(bsearch(&clone_ptr, original_ptrs, M, sizeof(int *), compare_pointers) == NULL);
             clone_idx++;
         }
         
         free(original_ptrs);
         int_sp_deinit(&original);
         int_sp_deinit(&clone);
}

static void test_big_clone_empty(void) {
    big_sp original;
    big_sp_init(&original);
    big_sp clone = big_sp_clone(&original);
    ASSERT(clone.count == 0);
    big_sp_deinit(&original);
    big_sp_deinit(&clone);
}

static void test_big_clone_single_element(void) {
    big_sp original;
    big_sp_init(&original);
    Big *orig_p = big_sp_put(&original, (Big){.i = 42});
    
    big_sp clone = big_sp_clone(&original);
    ASSERT(clone.count == 1);
    
    // Verify pointer differs but value matches
    Big *clone_p = &big_sp_begin(&clone).elm->value;
    ASSERT(clone_p != orig_p);
    ASSERT(clone_p->i == 42);
    
    // Modify original and ensure clone remains unchanged
    orig_p->i = 100;
    ASSERT(clone_p->i == 42);
    
    big_sp_deinit(&original);
    big_sp_deinit(&clone);
}

static void test_big_clone_multiple_elements(void) {
    big_sp original;
    big_sp_init(&original);
    const int N = 100;
    Big *original_ptrs[N];
    
    // Insert and track pointers
    for (int i = 0; i < N; ++i) {
        original_ptrs[i] = big_sp_put(&original, (Big){.i = i});
    }
    
    big_sp clone = big_sp_clone(&original);
    ASSERT(clone.count == N);
    
    // Collect clone pointers and verify uniqueness
    Big *clone_ptrs[N];
    size_t idx = 0;
    for (big_sp_iter_t it = big_sp_begin(&clone); 
         !big_sp_iter_eq(it, big_sp_end(&clone)); 
    it = big_sp_iter_next(it)) 
         {
             clone_ptrs[idx++] = big_sp_iter_elm(it);
         }
         
         // Sort for binary search
         qsort(original_ptrs, N, sizeof(Big *), compare_pointers);
         
         // Verify all clone pointers are new
         for (int i = 0; i < N; ++i) {
             ASSERT(bsearch(&clone_ptrs[i], original_ptrs, N, sizeof(Big *), compare_pointers) == NULL);
         }
         
         big_sp_deinit(&original);
         big_sp_deinit(&clone);
}

static void test_big_clone_independence_after_modification(void) {
    big_sp original;
    big_sp_init(&original);
    big_sp_put(&original, (Big){.i = 1});
    big_sp_put(&original, (Big){.i = 2});
    
    big_sp clone = big_sp_clone(&original);
    
    // Modify original after cloning
    big_sp_put(&original, (Big){.i = 3});
    
    ASSERT(original.count == 3);
    ASSERT(clone.count == 2);
    
    // Verify clone integrity
    struct Collector c = {0};
    big_sp_foreach(&clone, collect_big, &c);
    qsort(c.data, c.idx, sizeof(int), compare_ints);
    ASSERT(c.idx == 2);
    ASSERT(c.data[0] == 1);
    ASSERT(c.data[1] == 2);
    free(c.data);
    
    big_sp_deinit(&original);
    big_sp_deinit(&clone);
}

static void test_big_clone_with_holes(void) {
    big_sp original;
    big_sp_init(&original);
    Big *p1 = big_sp_put(&original, (Big){.i = 1});
    Big *p2 = big_sp_put(&original, (Big){.i = 2});
    Big *p3 = big_sp_put(&original, (Big){.i = 3});
    big_sp_pop(&original, p2);
    int *p1i = &p1->i;
    int *p2i = &p2->i;
    int *p3i = &p3->i;
    
    big_sp clone = big_sp_clone(&original);
    ASSERT(clone.count == 2);
    
    // Collect clone pointers
    int *clone_ptrs[2];
    size_t idx = 0;
    for (big_sp_iter_t it = big_sp_begin(&clone); 
         !big_sp_iter_eq(it, big_sp_end(&clone)); 
    it = big_sp_iter_next(it)) 
         {
             clone_ptrs[idx++] = &big_sp_iter_elm(it)->i;
         }
         
         // Verify no pointer matches original's remaining elements
         ASSERT(clone_ptrs[0] != p1i && clone_ptrs[0] != p3i);
         ASSERT(clone_ptrs[1] != p1i && clone_ptrs[1] != p3i);
         
         // Values should match
         qsort(clone_ptrs, 2, sizeof(int *), compare_int_ptrs);
         ASSERT(*clone_ptrs[0] == 1);
         ASSERT(*clone_ptrs[1] == 3);
         
         big_sp_deinit(&original);
         big_sp_deinit(&clone);
}

static void test_big_clone_deep_copy(void) {
    big_sp original;
    big_sp_init(&original);
    Big *orig_p = big_sp_put(&original, (Big){.i = 42});
    
    big_sp clone = big_sp_clone(&original);
    
    // Modify original element
    orig_p->i = 100;
    
    // Verify clone remains unchanged
    Big *clone_p = &big_sp_begin(&clone).elm->value;
    ASSERT(clone_p->i == 42);
    
    big_sp_deinit(&original);
    big_sp_deinit(&clone);
}

static void test_big_clone_stress(void) {
    big_sp original;
    big_sp_init(&original);
    const int M = 5000;
    Big **original_ptrs = malloc(M * sizeof(Big *));
    ASSERT(original_ptrs != NULL);
    
    // Insert and track all original pointers
    for (int i = 0; i < M; ++i) {
        original_ptrs[i] = big_sp_put(&original, (Big){.i = i});
    }
    
    big_sp clone = big_sp_clone(&original);
    ASSERT(clone.count == M);
    
    // Prepare for pointer comparison
    qsort(original_ptrs, M, sizeof(Big *), compare_pointers);
    
    // Verify clone pointers are unique and values match
    size_t clone_idx = 0;
    for (big_sp_iter_t it = big_sp_begin(&clone); 
         !big_sp_iter_eq(it, big_sp_end(&clone)); 
    it = big_sp_iter_next(it)) 
         {
             Big *clone_ptr = big_sp_iter_elm(it);
             
             // Pointer uniqueness check
             ASSERT(bsearch(&clone_ptr, original_ptrs, M, sizeof(Big *), compare_pointers) == NULL);
             
             clone_idx++;
         }
         
         free(original_ptrs);
         big_sp_deinit(&original);
         big_sp_deinit(&clone);
}

static void test_big_clone_after_original_deinit(void) {
    big_sp original;
    big_sp_init(&original);
    big_sp_put(&original, (Big){.i = 42});
    
    big_sp clone = big_sp_clone(&original);
    big_sp_deinit(&original);  // Destroy original
    
    // Verify clone remains valid
    ASSERT(clone.count == 1);
    Big *clone_p = &big_sp_begin(&clone).elm->value;
    ASSERT(clone_p->i == 42);
    
    big_sp_deinit(&clone);
}

static Big random_big(void) {
    constexpr int MAX_VALUE = 100000;
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

static void test_stable_pool(void) {
    srand(69);
    constexpr int NUM_OPS = 1000000;
    big_sp pool;
    big_sp_init(&pool);
    
    Big *vals = NULL;    // array of values for verification
    Big **ptrs = NULL;   // array of pointers to pool elements
    
    for (size_t op = 0; op < NUM_OPS; ++op) {
        if (arrlen(ptrs) == 0 || (rand() & 1)) {
            // PUT operation: insert new element
            Big new_el = random_big();
            Big *sp_ptr = big_sp_put(&pool, new_el);
            ASSERT(sp_ptr && "Failed to insert into stable_pool");
            // Track value and pointer
            arrpush(vals, *sp_ptr);
            arrpush(ptrs, sp_ptr);
        } else {
            // POP operation: remove random element via its pointer
            size_t idx = rand() % arrlen(ptrs);
            Big *target = ptrs[idx];
            // Copy value before removal
            Big expected = *target;
            // Remove from pool, get iterator to next
            big_sp_iter_t next_it = big_sp_pop(&pool, target);
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
            for (big_sp_iter_t it = big_sp_begin(&pool);
                 !big_sp_iter_is_end(it);
            it = big_sp_iter_next(it)) {
                ++count;
            }
            ASSERT(count == arrlen(vals) && "Size mismatch between pool and array");
            
            // Ensure every ptr in ptrs points to a valid element matching vals
            for (size_t i = 0; i < arrlen(ptrs); ++i) {
                Big *p = ptrs[i];
                int found = 0;
                for (big_sp_iter_t it = big_sp_begin(&pool);
                     !big_sp_iter_is_end(it);
                it = big_sp_iter_next(it)) {
                    if (big_equal(p, big_sp_iter_elm(it))) {
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
    
    big_sp_deinit(&pool);
    arrfree(vals);
    arrfree(ptrs);
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
    test_empty_iteration();
    test_int_erase_single_element();
    test_int_erase_first_element();
    test_int_erase_last_element_during_iteration();
    test_int_erase_every_other_element();
    test_int_stress_iter_pop();
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
    test_big_pop_and_iteration();
    test_big_pop_invalid_pointer();
    test_big_stress_inserts_pops();
    test_big_iteration_equivalence_after_random_pops();
    test_big_against_dynamic_array();
    test_big_insert_after_erase();
    test_clear_bucket();
    test_big_empty_iteration();
    test_big_erase_single_element();
    test_big_erase_first_element();
    test_big_erase_last_element_during_iteration();
    test_big_erase_every_other_element();
    test_big_stress_iter_pop();
    test_big_clone_empty();
    test_big_clone_single_element();
    test_big_clone_multiple_elements();
    test_big_clone_independence_after_modification();
    test_big_clone_with_holes();
    test_big_clone_deep_copy();
    test_big_clone_stress();
    test_big_clone_after_original_deinit();
    test_stable_pool();
    
    printf("ALL PASSED\n");
    return 0;
}

