#include <list>
#include <iterator>
#include <random>
#include <boost/container/stable_vector.hpp>
#include "plf_colony.h"
#include "plf_list.h"
#include "benchmark/benchmark.h"

#define printf(...)
// printf

typedef struct Big
{
    char _m[512 - 4];
    int i;

    bool operator==(const Big& other) const {
        return this->i == other.i;
    }
} Big;

uint64_t hash_big(Big b)
{
    return b.i;
}

bool eq_big(Big a, Big b)
{
    return a.i == b.i;
}

#define NAME big_set
#define KEY_TY Big
#define HASH_FN hash_big
#define CMPR_FN eq_big
#include "verstable.h"

extern "C"
{
#define SL_IMPL
#define SL_TYPE Big
#define SL_NAME big_sl

#include "step_list.h"
}

static void BM_stable_vector_iteration(benchmark::State& state)
{
    srand(69420);
    boost::container::stable_vector<Big> ls;
    const int N = state.range(0);
    
    std::vector<decltype(ls.begin())> elms;
    for (int i = 0; i < N; ++i) {
        Big b;
        b.i = rand() - i;
        ls.push_back(b);
        auto lsit = ls.begin();
        std::advance(lsit, i);
        elms.push_back(lsit);
    }
    
    std::mt19937 rng(42);
    std::vector<decltype(ls.begin())> to_erase;
    to_erase.reserve(N/2);
    std::sample(elms.begin(), elms.end(),
                std::back_inserter(to_erase),
                N/2,
                rng);
    
    for (auto it : to_erase)
    {
        ls.erase(it);
    }
    
    volatile unsigned int sum = 0;
    for (auto _ : state)
    {
        for (const auto& value : ls)
        {
            sum += value.i;
        }
        
        benchmark::ClobberMemory();
        benchmark::DoNotOptimize(sum);
        
        printf("list sum = %u\n", sum);
    }
}

void accumSum(Big *elm, void *arg)
{
    *(int*)arg += elm->i;
}

static void BM_set_iteration(benchmark::State& state)
{
    srand(69420);
    big_set set;
    big_set_init(&set);
    const int N = state.range(0);
    
    std::vector<Big> elms;
    for (int i = 0; i < N; ++i)
    {
        Big b;
        b.i = rand() - i;
        big_set_itr lsit = big_set_insert(&set, b);
        elms.push_back(b);
    }
    
    std::mt19937 rng(42);
    std::vector<Big> to_erase;
    to_erase.reserve(N/2);
    std::sample(elms.begin(), elms.end(),
                std::back_inserter(to_erase),
                N/2,
                rng);
    
    for (auto it : to_erase)
    {
        Big bsi = it;
        big_set_erase(&set, bsi);
    }
    
    volatile unsigned int sum = 0;
    for (auto _ : state)
    {
        for (big_set_itr itr = big_set_first( &set );
             !big_set_is_end( itr );
             itr = big_set_next( itr ))
        {
            sum += itr.data->key.i;
        }
    
        benchmark::ClobberMemory();
        benchmark::DoNotOptimize(sum);
        
        printf("set sum = %u\n", sum);
    }
    
    big_set_cleanup(&set);
}

static void BM_List_Iteration(benchmark::State& state)
{
    srand(69420);
    std::list<Big> ls;
    const int N = state.range(0);
    
    std::vector<decltype(ls.begin())> elms;
    for (int i = 0; i < N; ++i) {
        Big b;
        b.i = rand() - i;
        ls.push_back(b);
        auto lsit = ls.begin();
        std::advance(lsit, i);
        elms.push_back(lsit);
    }
    
    std::mt19937 rng(42);
    std::vector<decltype(ls.begin())> to_erase;
    to_erase.reserve(N/2);
    std::sample(elms.begin(), elms.end(),
                std::back_inserter(to_erase),
                N/2,
                rng);
    
    for (auto it : to_erase)
    {
        ls.erase(it);
    }
    
    volatile unsigned int sum = 0;
    for (auto _ : state)
    {
        for (const auto& value : ls)
        {
            sum += value.i;
        }
        
        benchmark::ClobberMemory();
        benchmark::DoNotOptimize(sum);
        
        printf("list sum = %u\n", sum);
    }
}

static void BM_PLFList_Iteration(benchmark::State& state)
{
    srand(69420);
    plf::list<Big> ls;
    const int N = state.range(0);
    
    std::vector<decltype(ls.begin())> elms;
    for (int i = 0; i < N; ++i) {
        Big b;
        b.i = rand() - i;
        ls.push_back(b);
        auto lsit = ls.begin();
        std::advance(lsit, i);
        elms.push_back(lsit);
    }
    
    std::mt19937 rng(42);
    std::vector<decltype(ls.begin())> to_erase;
    to_erase.reserve(N/2);
    std::sample(elms.begin(), elms.end(),
                std::back_inserter(to_erase),
                N/2,
                rng);
    
    for (auto it : to_erase)
    {
        ls.erase(it);
    }
    
    volatile unsigned int sum = 0;
    for (auto _ : state)
    {
        for (const auto& value : ls)
        {
            sum += value.i;
        }
        
        benchmark::ClobberMemory();
        benchmark::DoNotOptimize(sum);
        
        printf("list sum = %u\n", sum);
    }
}


static void BM_step_list(benchmark::State& state) {
    // Perform setup here
    srand(69420);
    big_sl sl; big_sl_init(&sl);
    const int M = state.range(0);
    Big **ptrs = (Big**) malloc(M * sizeof(ptrs[0]));
    for (int i = 0; i < M; i++)
        ptrs[i] = big_sl_put(&sl, (Big){.i = rand() - i});

    std::mt19937 rng(42);
    std::vector<Big*> to_pop;
    to_pop.reserve(M / 2);
    std::sample(ptrs, ptrs + M,
                std::back_inserter(to_pop),
                M / 2,
                rng);

    for (Big* p : to_pop) {
        big_sl_pop(&sl, p);
    }
    
    volatile unsigned int sum = 0;
    big_sl *ssl = &sl;
    for (auto _ : state) {
        
        SL_FOREACH(ssl, sum += SL_IT->i; );
        
        benchmark::ClobberMemory();
        benchmark::DoNotOptimize(sum);

        printf("stepsum = %u\n", sum);
    }
    
    free(ptrs);
    big_sl_deinit(&sl);
}

static void BM_step_list_iter(benchmark::State& state) {
    // Perform setup here
    srand(69420);
    big_sl sl; big_sl_init(&sl);
    const int M = state.range(0);
    Big **ptrs = (Big**) malloc(M * sizeof(ptrs[0]));
    for (int i = 0; i < M; i++)
        ptrs[i] = big_sl_put(&sl, (Big){.i = rand() - i});
    
    std::mt19937 rng(42);
    std::vector<Big*> to_pop;
    to_pop.reserve(M / 2);
    std::sample(ptrs, ptrs + M,
                std::back_inserter(to_pop),
                M / 2,
                rng);

    for (Big* p : to_pop) {
        big_sl_pop(&sl, p);
    }


    volatile unsigned int sum = 0;
    big_sl *ssl = &sl;
    for (auto _ : state) {
        
        for(big_sl_iter_t it  = big_sl_begin(&sl),
                          end = big_sl_end(&sl)  ;
            !big_sl_iter_eq(it, end)             ;
            it = big_sl_iter_next(it) )
        {
            sum += big_sl_iter_elm(it)->i;
        }
        
        benchmark::ClobberMemory();
        benchmark::DoNotOptimize(sum);
        
         printf("step_it_sum = %u\n", sum);
    }
    
    free(ptrs);
    big_sl_deinit(&sl);
}

void add_sum(Big *big, void *arg)
{
    *(unsigned int*)arg += big->i;
}

static void BM_step_list_func(benchmark::State& state) {
    // Perform setup here
    srand(69420);
    big_sl sl; big_sl_init(&sl);
    const int M = state.range(0);
    Big **ptrs = (Big**) malloc(M * sizeof(ptrs[0]));
    for (int i = 0; i < M; i++)
        ptrs[i] = big_sl_put(&sl, (Big){.i = rand() - i});

    std::mt19937 rng(42);
    std::vector<Big*> to_pop;
    to_pop.reserve(M / 2);
    std::sample(ptrs, ptrs + M,
                std::back_inserter(to_pop),
                M / 2,
                rng);
    for (Big* p : to_pop) {
        big_sl_pop(&sl, p);
    }
    volatile unsigned int sum = 0;
    big_sl *ssl = &sl;
    // printf("sl size = %zu\n", sl.count);
    for (auto _ : state) {
        
        big_sl_foreach(ssl, add_sum, (void*)&sum);
        
        benchmark::ClobberMemory();
        benchmark::DoNotOptimize(sum);
        
         printf("step_func_sum = %u\n", sum);
    }
    
    free(ptrs);
    big_sl_deinit(&sl);
}

static void BM_PLFColony_Iteration(benchmark::State& state)
{
    srand(69420);
    plf::colony<Big> i_colony;
    const int N = state.range(0);

    std::vector<decltype(i_colony.begin())> elms;
    elms.reserve(N);
    for (int i = 0; i < N; ++i)
    {
        elms.push_back(i_colony.insert((Big){.i = rand() - i}));
    }

    std::mt19937 rng(42);
    std::vector<decltype(i_colony.begin())> to_erase;
    to_erase.reserve(N/2);
    std::sample(elms.begin(), elms.end(),
                std::back_inserter(to_erase),
                N/2,
                rng);

    for (auto it : to_erase)
    {
        i_colony.erase(it);
    }

    volatile unsigned int sum = 0;
    for (auto _ : state)
    {
        for (const auto& value : i_colony)
        {
            sum += value.i;
        }
        benchmark::ClobberMemory();
        benchmark::DoNotOptimize(sum);
        printf("plfsum = %u\n", sum);
    }
}

//BENCHMARK(BM_List_Iteration)->RangeMultiplier(2)->     Range(512, 512 * 64);
BENCHMARK(BM_step_list)              ->RangeMultiplier(2)->Range(2048, 2048 * 512);
BENCHMARK(BM_step_list_func)         ->RangeMultiplier(2)->Range(2048, 2048 * 512);
BENCHMARK(BM_step_list_iter)         ->RangeMultiplier(2)->Range(2048, 2048 * 512);
BENCHMARK(BM_PLFColony_Iteration)    ->RangeMultiplier(2)->Range(2048, 2048 * 512);
//BENCHMARK(BM_stable_vector_iteration)->RangeMultiplier(2)->Range(2048, 2048 * 512);
//BENCHMARK(BM_PLFList_Iteration)      ->RangeMultiplier(2)->Range(2048, 2048 * 512);

BENCHMARK_MAIN();

struct CompareContext {
    const plf::colony<Big>& vec;
    bool match = true;
};

static void compare_int(Big* v, void* arg) {
    CompareContext* ctx = static_cast<CompareContext*>(arg);
    if (std::find(ctx->vec.begin(), ctx->vec.end(), *v) == ctx->vec.end())
    {
        ctx->match = false;
    }
}

bool step_list_equals_vector(const big_sl* sl, const plf::colony<Big>& vec) {
    if (sl->count != vec.size()) {
        return false;
    }
    CompareContext ctx{vec};
    big_sl_foreach(sl, compare_int, (void*) &ctx);
    return ctx.match;
}
