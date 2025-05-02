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
    struct Collector c = {NULL, 0, 0};
    int_sl_loop(&sl, collect_int, &c);
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
    
    struct Collector c = {NULL, 0, 0};
    int_sl_loop(&sl, collect_int, &c);
    ASSERT(c.idx == (size_t)N);
    
    for (int expected = 0; expected < N; expected++) {
        int found = 0;
        for (size_t j = 0; j < c.idx; j++) {
            if (c.data[j] == expected) { found = 1; break; }
        }
        ASSERT(found);
    }
    free(c.data);
    int_sl_deinit(&sl);
}


static void test_pointer_stability(void) {
    int_sl sl; int_sl_init(&sl);
    int *p1 = int_sl_put(&sl, 10);
    int *p2 = int_sl_put(&sl, 20);
    int *p3 = int_sl_put(&sl, 30);
    int_sl_pop(&sl, p3);
    ASSERT(*p1 == 10);
    ASSERT(*p2 == 20);
    int_sl_deinit(&sl);
}


static void test_pop_and_iteration(void) {
    int_sl sl; int_sl_init(&sl);
    int *a = int_sl_put(&sl, 1);
    int *b = int_sl_put(&sl, 2);
    int *c = int_sl_put(&sl, 3);
    int_sl_pop(&sl, b);
    
    struct Collector col = {NULL, 0, 0};
    int_sl_loop(&sl, collect_int, &col);
    ASSERT(col.idx == 2);
    for (size_t i = 0; i < col.idx; i++)
        ASSERT(col.data[i] != 2);
    free(col.data);
    int_sl_deinit(&sl);
}


static void test_pop_invalid_pointer(void) {
    int_sl sl; int_sl_init(&sl);
    int dummy = 0;
    int_sl_pop(&sl, &dummy);  
    int_sl_deinit(&sl);
}

static void test_stress_inserts_pops(void) {
    int_sl sl; int_sl_init(&sl);
    const int M = 10000;
    int **ptrs = malloc(M * sizeof *ptrs);
    ASSERT(ptrs != NULL);
    for (int i = 0; i < M; i++)
        ptrs[i] = int_sl_put(&sl, i);
    for (int i = 0; i < M; i += 2)
        int_sl_pop(&sl, ptrs[i]);
    
    struct Collector c = {NULL, 0, 0};
    int_sl_loop(&sl, collect_int, &c);
    ASSERT(c.idx == (size_t)(M/2));
    
    for (int expected = 1; expected < M; expected += 2) {
        int found = 0;
        for (size_t j = 0; j < c.idx; j++) {
            if (c.data[j] == expected) { found = 1; break; }
        }
        ASSERT(found);
    }
    free(c.data);
    free(ptrs);
    int_sl_deinit(&sl);
}

static void test_smaller_stress_inserts_pops(void) {
    int_sl sl; int_sl_init(&sl);
    const int M = 500;
    int **ptrs = malloc(M * sizeof *ptrs);
    ASSERT(ptrs != NULL);
    
    // Insert M elements
    for (int i = 0; i < M; i++) {
        ptrs[i] = int_sl_put(&sl, i);
        ASSERT(ptrs[i] != NULL);
    }
    
    // Pop every other element (25 pops)
    for (int i = 0; i < M; i += 2) {
        int_sl_pop(&sl, ptrs[i]);
    }
    
    // Verify remaining elements
    struct Collector c = {NULL, 0, 0};
    int_sl_loop(&sl, collect_int, &c);
    ASSERT(c.idx == (M/2));  // Should have 25 elements remaining
    
    // Check all odd numbers 1, 3, 5...49 exist
    for (int expected = 1; expected < M; expected += 2) {
        int found = 0;
        for (size_t j = 0; j < c.idx; j++) {
            if (c.data[j] == expected) {
                found = 1;
                break;
            }
        }
        ASSERT(found);
    }
    
    free(c.data);
    free(ptrs);
    int_sl_deinit(&sl);
}

// Add this type definition at the top
typedef struct Big {
    char _m[256 - 4];
    int i;
} Big;

// New list type for Big
#define SL_TYPE Big
#define SL_NAME big_sl
#include "step_list.h"

// Modified collector for Big type
static void collect_big(Big *v, void *arg) {
    struct Collector *c = arg;
    if (c->idx == c->cap) {
        size_t new_cap = c->cap ? c->cap * 2 : 16;
        int *tmp = realloc(c->data, new_cap * sizeof *tmp);
        ASSERT(tmp != NULL);
        c->data = tmp;
        c->cap = new_cap;
    }
    c->data[c->idx++] = v->i;  // Collect just the integer field
}

// Modified test_init_deinit
static void test_big_init_deinit(void) {
    big_sl sl;
    big_sl_init(&sl);
    ASSERT(sl.count == 0);
    big_sl_deinit(&sl);
}

// Modified test_single_put_and_loop
static void test_big_single_put_and_loop(void) {
    big_sl sl; big_sl_init(&sl);
    Big *p = big_sl_put(&sl, (Big){.i = 42});
    ASSERT(p && p->i == 42);

    struct Collector c = {NULL, 0, 0};
    big_sl_loop(&sl, collect_big, &c);
    ASSERT(c.idx == 1);
    ASSERT(c.data[0] == 42);
    free(c.data);
    big_sl_deinit(&sl);
}

// Modified test_multiple_puts
static void test_big_multiple_puts(void) {
    big_sl sl; big_sl_init(&sl);
    const int N = 100;

    for (int i = 0; i < N; i++) {
        Big *p = big_sl_put(&sl, (Big){.i = i});
        ASSERT(p != NULL);
    }

    struct Collector c = {NULL, 0, 0};
    big_sl_loop(&sl, collect_big, &c);
    ASSERT(c.idx == (size_t)N);

    for (int expected = 0; expected < N; expected++) {
        int found = 0;
        for (size_t j = 0; j < c.idx; j++) {
            if (c.data[j] == expected) { found = 1; break; }
        }
        ASSERT(found);
    }
    free(c.data);
    big_sl_deinit(&sl);
}

// Modified test_pointer_stability
static void test_big_pointer_stability(void) {
    big_sl sl; big_sl_init(&sl);
    Big *p1 = big_sl_put(&sl, (Big){.i = 10});
    Big *p2 = big_sl_put(&sl, (Big){.i = 20});
    Big *p3 = big_sl_put(&sl, (Big){.i = 30});
    big_sl_pop(&sl, p3);
    ASSERT(p1->i == 10);
    ASSERT(p2->i == 20);
    big_sl_deinit(&sl);
}

// Modified test_pop_and_iteration
static void test_big_pop_and_iteration(void) {
    big_sl sl; big_sl_init(&sl);
    Big *a = big_sl_put(&sl, (Big){.i = 1});
    Big *b = big_sl_put(&sl, (Big){.i = 2});
    Big *c = big_sl_put(&sl, (Big){.i = 3});
    big_sl_pop(&sl, b);

    struct Collector col = {NULL, 0, 0};
    big_sl_loop(&sl, collect_big, &col);
    ASSERT(col.idx == 2);
    for (size_t i = 0; i < col.idx; i++)
        ASSERT(col.data[i] != 2);
    free(col.data);
    big_sl_deinit(&sl);
}

// Modified test_pop_invalid_pointer
static void test_big_pop_invalid_pointer(void) {
    big_sl sl; big_sl_init(&sl);
    Big dummy = {.i = -1};
    big_sl_pop(&sl, &dummy);  // Shouldn't crash
    big_sl_deinit(&sl);
}

// Modified stress test for Big
static void test_big_stress_inserts_pops(void) {
    big_sl sl; big_sl_init(&sl);
    const int M = 2048;
    Big **ptrs = malloc(M * sizeof *ptrs);
    ASSERT(ptrs != NULL);

    for (int i = 0; i < M; i++) {
        ptrs[i] = big_sl_put(&sl, (Big){.i = i});
        ASSERT(ptrs[i] != NULL);
    }

    // Pop every other element
    for (int i = 0; i < M; i += 2) {
        big_sl_pop(&sl, ptrs[i]);
    }

    struct Collector col = {NULL, 0, 0};
    // big_sl_loop(&sl, collect_big, &col);

    big_sl *bsl = &sl;
    SL_FOREACH(bsl)
    {
        collect_big(SL_IT, &col);
    }

    ASSERT(col.idx == M/2);  // Half of 50

    // Verify remaining numbers
    for (int expected = 1; expected < M; expected += 2) {
        int found = 0;
        for (size_t j = 0; j < col.idx; j++) {
            if (col.data[j] == expected) {
                found = 1;
                break;
            }
        }
        ASSERT(found);
    }

    free(col.data);
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

    printf("\nRunning Big struct tests...\n");
    test_big_init_deinit();
    test_big_single_put_and_loop();
    test_big_multiple_puts();
    test_big_pointer_stability();
    test_big_pop_and_iteration();
    test_big_pop_invalid_pointer();
    test_big_stress_inserts_pops();

    printf("ALL PASSED\n");
    return 0;
}
