#include <stdio.h>
#include <stdlib.h>

#define ASSERT(expr)                                                          \
do {                                                                      \
    if (!(expr)) {                                                        \
        fprintf(stderr,                                                   \
        "ASSERT FAILED: %s\n  at %s:%d in %s()\n",            \
        #expr, __FILE__, __LINE__, __func__);                    \
        exit(EXIT_FAILURE);                                               \
    }                                                                     \
} while (0)

// Helper function for sorting integers
static int compare_ints(const void *a, const void *b) {
    int arg1 = *(const int *)a;
    int arg2 = *(const int *)b;
    return (arg1 > arg2) - (arg1 < arg2);
}

#define SL_IMPL
#define SL_TYPE int
#define SL_NAME int_sl
#include "step_list.h"

struct Collector {
    int *data;
    size_t idx, cap;
};

static void collect_int(int *v, void *arg) {
    struct Collector *c = arg;
    if (c->idx == c->cap) {
        size_t new_cap = c->cap ? c->cap * 2 : 16;
        int *tmp = realloc(c->data, new_cap * sizeof *tmp);
        ASSERT(tmp != NULL);
        c->data = tmp;
        c->cap = new_cap;
    }
    c->data[c->idx++] = *v;
}

static void test_init_deinit(void) {
    int_sl sl;
    int_sl_init(&sl);
    ASSERT(sl.count == 0);
    int_sl_deinit(&sl);
}

static void test_single_put_and_loop(void) {
    int_sl sl; int_sl_init(&sl);
    int *p = int_sl_put(&sl, 42);
    ASSERT(p && *p == 42);
    ASSERT(sl.count == (size_t)1); // Check count after insertion
    struct Collector c = {NULL, 0, 0};
    int_sl_foreach(&sl, collect_int, &c);
    ASSERT(c.idx == 1);
    ASSERT(c.data[0] == 42);
    free(c.data);
    int_sl_deinit(&sl);
}

static void test_multiple_puts(void) {
    int_sl sl; int_sl_init(&sl);
    const int N = 100;
    for (int i = 0; i < N; i++)
        ASSERT(int_sl_put(&sl, i) != NULL);
    ASSERT(sl.count == (size_t)N); // Check count
    
    struct Collector c = {NULL, 0, 0};
    int_sl_foreach(&sl, collect_int, &c);
    ASSERT(c.idx == (size_t)N);
    
    qsort(c.data, c.idx, sizeof(int), compare_ints); // Sort collected data
    for (size_t i = 0; i < c.idx; i++)
        ASSERT(c.data[i] == (int)i); // Check sorted values
        
    free(c.data);
    int_sl_deinit(&sl);
}

static void test_pointer_stability(void) {
    int_sl sl; int_sl_init(&sl);
    int *p1 = int_sl_put(&sl, 10);
    int *p2 = int_sl_put(&sl, 20);
    int *p3 = int_sl_put(&sl, 30);
    ASSERT(sl.count == 3);
    int_sl_pop(&sl, p3);
    ASSERT(sl.count == 2); // Check count after pop
    ASSERT(*p1 == 10);
    ASSERT(*p2 == 20);
    int_sl_deinit(&sl);
}

static void test_pop_and_iteration(void) {
    int_sl sl; int_sl_init(&sl);
    int *a = int_sl_put(&sl, 1);
    int *b = int_sl_put(&sl, 2);
    int *c = int_sl_put(&sl, 3);
    ASSERT(sl.count == 3);
    int_sl_pop(&sl, b);
    ASSERT(sl.count == 2); // Check count after pop
    
    struct Collector col = {NULL, 0, 0};
    int_sl_foreach(&sl, collect_int, &col);
    ASSERT(col.idx == 2);
    
    qsort(col.data, col.idx, sizeof(int), compare_ints); // Sort and check values
    ASSERT(col.data[0] == 1);
    ASSERT(col.data[1] == 3);
    
    free(col.data);
    int_sl_deinit(&sl);
    (void)a, (void)b, (void)c;
}

static void test_pop_invalid_pointer(void) {
    int_sl sl; int_sl_init(&sl);
    int dummy = 0;
    ASSERT(sl.count == 0);
    int_sl_pop(&sl, &dummy);
    ASSERT(sl.count == 0); // Ensure count remains 0
    int_sl_deinit(&sl);
}

static void test_stress_inserts_pops(void) {
    int_sl sl; int_sl_init(&sl);
    const int M = 10000;
    int **ptrs = malloc(M * sizeof *ptrs);
    ASSERT(ptrs != NULL);
    for (int i = 0; i < M; i++)
        ptrs[i] = int_sl_put(&sl, i);
    ASSERT(sl.count == (size_t)M);
    
    for (int i = 0; i < M; i += 2)
        int_sl_pop(&sl, ptrs[i]);
    ASSERT(sl.count == (size_t)M/2); // Check count after pops
    
    struct Collector c = {NULL, 0, 0};
    int_sl_foreach(&sl, collect_int, &c);
    ASSERT(c.idx == (size_t)(M/2));
    
    qsort(c.data, c.idx, sizeof(int), compare_ints); // Sort and check
    for (size_t i = 0; i < c.idx; i++)
        ASSERT(c.data[i] == 1 + 2*(int)i);
    
    free(c.data);
    free(ptrs);
    int_sl_deinit(&sl);
}

static void test_smaller_stress_inserts_pops(void) {
    int_sl sl; int_sl_init(&sl);
    const int M = 500;
    int **ptrs = malloc(M * sizeof *ptrs);
    ASSERT(ptrs != NULL);
    
    for (int i = 0; i < M; i++) {
        ptrs[i] = int_sl_put(&sl, i);
        ASSERT(ptrs[i] != NULL);
    }
    ASSERT(sl.count == (size_t)M);
    
    for (int i = 0; i < M; i += 2)
        int_sl_pop(&sl, ptrs[i]);
    ASSERT(sl.count == (size_t)M/2);
    
    struct Collector c = {NULL, 0, 0};
    int_sl_foreach(&sl, collect_int, &c);
    ASSERT(c.idx == (size_t)(M/2));
    
    qsort(c.data, c.idx, sizeof(int), compare_ints); // Sort and check
    for (size_t i = 0; i < c.idx; i++)
        ASSERT(c.data[i] == 1 + 2*(int)i);
    
    free(c.data);
    free(ptrs);
    int_sl_deinit(&sl);
}

// Big struct tests (similar modifications applied)

typedef struct Big {
    char _m[256 - 4];
    int i;
} Big;

#define SL_IMPL
#define SL_TYPE Big
#define SL_NAME big_sl
#include "step_list.h"

static void collect_big(Big *v, void *arg) {
    struct Collector *c = arg;
    if (c->idx == c->cap) {
        size_t new_cap = c->cap ? c->cap * 2 : 16;
        int *tmp = realloc(c->data, new_cap * sizeof *tmp);
        ASSERT(tmp != NULL);
        c->data = tmp;
        c->cap = new_cap;
    }
    c->data[c->idx++] = v->i;
}

static void test_big_init_deinit(void) {
    big_sl sl;
    big_sl_init(&sl);
    ASSERT(sl.count == 0);
    big_sl_deinit(&sl);
}

static void test_big_single_put_and_loop(void) {
    big_sl sl; big_sl_init(&sl);
    Big *p = big_sl_put(&sl, (Big){.i = 42});
    ASSERT(p && p->i == 42);
    ASSERT(sl.count == 1);
    
    struct Collector c = {NULL, 0, 0};
    SL_FOREACH(&sl, collect_big(SL_IT, &c););
    ASSERT(c.idx == 1);
    ASSERT(c.data[0] == 42);
    free(c.data);
    big_sl_deinit(&sl);
}

static void test_big_multiple_puts(void) {
    big_sl sl; big_sl_init(&sl);
    const int N = 100;
    for (int i = 0; i < N; i++) {
        Big *p = big_sl_put(&sl, (Big){.i = i});
        ASSERT(p != NULL);
    }
    ASSERT(sl.count == (size_t)N);
    
    struct Collector c = {NULL, 0, 0};
    for(big_sl_iter_t it = big_sl_begin(&sl), end = big_sl_end(&sl) ; !big_sl_iter_eq(it, end) ; it = big_sl_iter_next(it))
    {
        collect_big(big_sl_iter_elm(it), &c);
    }
    ASSERT(c.idx == (size_t)N);
    
    qsort(c.data, c.idx, sizeof(int), compare_ints);
    for (size_t i = 0; i < c.idx; i++)
        ASSERT(c.data[i] == (int)i);
    
    free(c.data);
    big_sl_deinit(&sl);
}

static void test_big_pointer_stability(void) {
    big_sl sl; big_sl_init(&sl);
    Big *p1 = big_sl_put(&sl, (Big){.i = 10});
    Big *p2 = big_sl_put(&sl, (Big){.i = 20});
    Big *p3 = big_sl_put(&sl, (Big){.i = 30});
    ASSERT(sl.count == 3);
    big_sl_pop(&sl, p3);
    ASSERT(sl.count == 2);
    ASSERT(p1->i == 10);
    ASSERT(p2->i == 20);
    big_sl_deinit(&sl);
}

static void test_big_pop_and_iteration(void) {
    big_sl sl; big_sl_init(&sl);
    Big *a = big_sl_put(&sl, (Big){.i = 1});
    Big *b = big_sl_put(&sl, (Big){.i = 2});
    Big *c = big_sl_put(&sl, (Big){.i = 3});
    ASSERT(sl.count == 3);
    big_sl_pop(&sl, b);
    ASSERT(sl.count == 2);
    
    struct Collector col = {NULL, 0, 0};
    SL_FOREACH(&sl, collect_big(SL_IT, &col); );
    ASSERT(col.idx == 2);
    
    qsort(col.data, col.idx, sizeof(int), compare_ints);
    ASSERT(col.data[0] == 1);
    ASSERT(col.data[1] == 3);
    
    free(col.data);
    big_sl_deinit(&sl);
    
    (void)a,(void)b,(void)c;
}

static void test_big_pop_invalid_pointer(void) {
    big_sl sl; big_sl_init(&sl);
    Big dummy = {.i = -1};
    ASSERT(sl.count == 0);
    big_sl_pop(&sl, &dummy);
    ASSERT(sl.count == 0);
    big_sl_deinit(&sl);
}

static void test_big_stress_inserts_pops(void) {
    big_sl sl; big_sl_init(&sl);
    const int M = 2048 * 32;
    Big **ptrs = malloc(M * sizeof *ptrs);
    ASSERT(ptrs != NULL);
    
    for (int i = 0; i < M; i++) {
        ptrs[i] = big_sl_put(&sl, (Big){.i = i});
        ASSERT(ptrs[i] != NULL);
    }
    ASSERT(sl.count == (size_t) M);
    
    for (int i = 0; i < M; i += 2)
        big_sl_pop(&sl, ptrs[i]);
    ASSERT(sl.count == (size_t)M/2);
    
    struct Collector col = {NULL, 0, 0};
    for(big_sl_iter_t it = big_sl_begin(&sl), end = big_sl_end(&sl) ; !big_sl_iter_eq(it, end) ; it = big_sl_iter_next(it)) {
        collect_big(big_sl_iter_elm(it), &col);
    }
    ASSERT(col.idx == (size_t)M/2);
    
    qsort(col.data, col.idx, sizeof(int), compare_ints);
    for (size_t i = 0; i < col.idx; i++)
        ASSERT(col.data[i] == 1 + 2*(int)i);
    
    free(col.data);
    free(ptrs);
    big_sl_deinit(&sl);
}

static void test_int_iteration_equivalence_after_random_pops(void) {
    int_sl sl; int_sl_init(&sl);
    const int N = 1024;
    int **ptrs = malloc(N * sizeof *ptrs);
    ASSERT(ptrs != NULL);
    
    // Seed randomness and insert
    srand(42);
    for (int i = 0; i < N; i++)
        ptrs[i] = int_sl_put(&sl, i);
    
    // Randomly pop about half of them
    for (int i = 0; i < N; i++) {
        if (rand() % 2) {
            int_sl_pop(&sl, ptrs[i]);
            ptrs[i] = NULL;
        }
    }
    
    struct Collector col1 = {NULL, 0, 0};
    struct Collector col2 = {NULL, 0, 0};
    struct Collector col3 = {NULL, 0, 0};
    
    // Method 1: function pointer
    int_sl_foreach(&sl, collect_int, &col1);
    
    // Method 2: macro
    SL_FOREACH(&sl, collect_int(SL_IT, &col2););
    
    // Method 3: iterator
    for (int_sl_iter_t it = int_sl_begin(&sl), end = int_sl_end(&sl);
         !int_sl_iter_eq(it, end);
    it = int_sl_iter_next(it)) {
        collect_int(int_sl_iter_elm(it), &col3);
    }
    
    ASSERT(col1.idx == col2.idx && col2.idx == col3.idx);
    
    qsort(col1.data, col1.idx, sizeof(int), compare_ints);
    qsort(col2.data, col2.idx, sizeof(int), compare_ints);
    qsort(col3.data, col3.idx, sizeof(int), compare_ints);
    
    for (size_t i = 0; i < col1.idx; i++) {
        ASSERT(col1.data[i] == col2.data[i]);
        ASSERT(col2.data[i] == col3.data[i]);
    }
    
    free(col1.data);
    free(col2.data);
    free(col3.data);
    free(ptrs);
    int_sl_deinit(&sl);
}

static void test_big_iteration_equivalence_after_random_pops(void) {
    big_sl sl; big_sl_init(&sl);
    const int N = 1024;
    Big **ptrs = malloc(N * sizeof *ptrs);
    ASSERT(ptrs != NULL);
    
    srand(1337);
    for (int i = 0; i < N; i++)
        ptrs[i] = big_sl_put(&sl, (Big){.i = i});
    
    for (int i = 0; i < N; i++) {
        if (rand() % 2) {
            big_sl_pop(&sl, ptrs[i]);
            ptrs[i] = NULL;
        }
    }
    
    struct Collector col1 = {NULL, 0, 0};
    struct Collector col2 = {NULL, 0, 0};
    struct Collector col3 = {NULL, 0, 0};
    
    big_sl_foreach(&sl, collect_big, &col1);
    SL_FOREACH(&sl, collect_big(SL_IT, &col2););
    for (big_sl_iter_t it = big_sl_begin(&sl), end = big_sl_end(&sl);
         !big_sl_iter_eq(it, end);
    it = big_sl_iter_next(it)) {
        collect_big(big_sl_iter_elm(it), &col3);
    }
    
    ASSERT(col1.idx == col2.idx && col2.idx == col3.idx);
    
    qsort(col1.data, col1.idx, sizeof(int), compare_ints);
    qsort(col2.data, col2.idx, sizeof(int), compare_ints);
    qsort(col3.data, col3.idx, sizeof(int), compare_ints);
    
    for (size_t i = 0; i < col1.idx; i++) {
        ASSERT(col1.data[i] == col2.data[i]);
        ASSERT(col2.data[i] == col3.data[i]);
    }
    
    free(col1.data);
    free(col2.data);
    free(col3.data);
    free(ptrs);
    big_sl_deinit(&sl);
}

int main(void) {
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
    
    printf("\nRunning Big struct tests...\n");
    test_big_init_deinit();
    test_big_single_put_and_loop();
    test_big_multiple_puts();
    test_big_pointer_stability();
    test_big_pop_and_iteration();
    test_big_pop_invalid_pointer();
    test_big_stress_inserts_pops();
    test_big_iteration_equivalence_after_random_pops();
    
    printf("ALL PASSED\n");
    return 0;
}
