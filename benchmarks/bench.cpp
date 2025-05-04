#include <list>
#include <iterator>
#include <random>
#include "plf_colony.h"
#include "benchmark/benchmark.h"
// #include "cc.h"

typedef struct Big
{
    char _m[512 - 4];
    int i;

    bool operator==(const Big& other) const {
        return this->i == other.i;
    }
} Big;

extern "C"
{
#define SL_IMPL
#define SL_TYPE Big
#define SL_NAME big_sl

#include "step_list.h"
}

void accumSum(Big *elm, void *arg)
{
    *(int*)arg += elm->i;
}

// static void BM_set_iteration(benchmark::State& state)
// {
//     set(Big) s;
//     const int N = state.range(0);
//
//     std::vector<decltype(ls.begin())> elms;
//     for (int i = 0; i < N; ++i) {
//         Big b;
//         b.i = i;
//         ls.push_back(b);
//         auto lsit = ls.begin();
//         std::advance(lsit, i);
//         elms.push_back(lsit);
//     }
//
//     for(int i = 0 ; i < N ; i += 2)
//     {
//         for(auto it = ls.begin(); it != ls.end() ; it++)
//         {
//             if(it == elms[i])
//             {
//                 ls.erase(elms[i]);
//                 break;
//             }
//         }
//     }
//
//     // printf("std::list size = %zu\n", ls.size());
//     volatile unsigned int sum = 0;
//     for (auto _ : state)
//     {
//         for (const auto& value : ls)
//         {
//             sum += value.i;
//         }
//
//         benchmark::ClobberMemory();
//         benchmark::DoNotOptimize(sum);
//     }
// }

static void BM_List_Iteration(benchmark::State& state)
{
    std::list<Big> ls;
    const int N = state.range(0);
    
    std::vector<decltype(ls.begin())> elms;
    for (int i = 0; i < N; ++i) {
        Big b;
        b.i = i;
        ls.push_back(b);
        auto lsit = ls.begin();
        std::advance(lsit, i);
        elms.push_back(lsit);
    }
    
    for(int i = 0 ; i < N ; i += 2)
    {
        for(auto it = ls.begin(); it != ls.end() ; it++)
        {
            if(it == elms[i])
            {
                ls.erase(elms[i]);
                break;
            }
        }
    }
    
    // printf("std::list size = %zu\n", ls.size());
    volatile unsigned int sum = 0;
    for (auto _ : state)
    {
        for (const auto& value : ls)
        {
            sum += value.i;
        }
        
        benchmark::ClobberMemory();
        benchmark::DoNotOptimize(sum);
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

    std::mt19937 rng(42);  // fixed seed ⇒ reproducible sequence :contentReference[oaicite:2]{index=2}
    std::vector<Big*> to_pop;
    to_pop.reserve(M / 2); // avoid reallocations :contentReference[oaicite:3]{index=3}
    std::sample(ptrs, ptrs + M,
                std::back_inserter(to_pop),
                M / 2,
                rng);            // uniform sample without replacement :contentReference[oaicite:4]{index=4}

    for (Big* p : to_pop) {
        big_sl_pop(&sl, p);
    }
    
    volatile unsigned int sum = 0;
    big_sl *ssl = &sl;
    for (auto _ : state) {
        
        SL_FOREACH(ssl, sum += SL_IT->i; );
        
        benchmark::ClobberMemory();
        benchmark::DoNotOptimize(sum);

        //printf("stepsum = %d\n", sum);
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
    // for (int i = 0; i < M; i += 2)
    //     big_sl_pop(&sl, ptrs[i]);
    

    // 2) Deterministic random removal of M/2 elements
    std::mt19937 rng(42);  // fixed seed ⇒ reproducible sequence :contentReference[oaicite:2]{index=2}
    std::vector<Big*> to_pop;
    to_pop.reserve(M / 2); // avoid reallocations :contentReference[oaicite:3]{index=3}
    std::sample(ptrs, ptrs + M,
                std::back_inserter(to_pop),
                M / 2,
                rng);            // uniform sample without replacement :contentReference[oaicite:4]{index=4}

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
    }
    
    free(ptrs);
    big_sl_deinit(&sl);
}

void add_sum(Big *big, void *arg)
{
    *(int*)arg += big->i;
}

static void BM_step_list_func(benchmark::State& state) {
    // Perform setup here
    big_sl sl; big_sl_init(&sl);
    const int M = state.range(0);
    Big **ptrs = (Big**) malloc(M * sizeof(ptrs[0]));
    for (int i = 0; i < M; i++)
        ptrs[i] = big_sl_put(&sl, (Big){.i = i});

    std::mt19937 rng(42);  // fixed seed ⇒ reproducible sequence :contentReference[oaicite:2]{index=2}
    std::vector<Big*> to_pop;
    to_pop.reserve(M / 2); // avoid reallocations :contentReference[oaicite:3]{index=3}
    std::sample(ptrs, ptrs + M,
                std::back_inserter(to_pop),
                M / 2,
                rng);            // uniform sample without replacement :contentReference[oaicite:4]{index=4}
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
    }
    
    free(ptrs);
    big_sl_deinit(&sl);
}

static void BM_PLFColony_Iteration(benchmark::State& state)
{
    srand(69420);
    plf::colony<Big> i_colony;
    const int N = state.range(0);

    // 1) Insert N items with arbitrary values
    std::vector<decltype(i_colony.begin())> elms;
    elms.reserve(N);
    for (int i = 0; i < N; ++i)
    {
        elms.push_back(i_colony.insert((Big){.i = rand() - i}));
    }

    std::mt19937 rng(42);                            // fixed seed
    std::vector<decltype(i_colony.begin())> to_erase;
    to_erase.reserve(N/2);
    std::sample(elms.begin(), elms.end(),
                std::back_inserter(to_erase),
                N/2,
                rng);                            // C++17 std::sample :contentReference[oaicite:4]{index=4}

    for (auto it : to_erase)
    {
        i_colony.erase(it);
    }

    // 3) Benchmark iteration over remaining elements
    volatile unsigned int sum = 0;
    for (auto _ : state)
    {
        for (const auto& value : i_colony)
        {
            sum += value.i;
        }
        benchmark::ClobberMemory();
        benchmark::DoNotOptimize(sum);
        //printf("plfsum = %d\n", sum);
    }
}

//BENCHMARK(BM_List_Iteration)->RangeMultiplier(2)->Range(16, 2048 * 64);
BENCHMARK(BM_step_list)->RangeMultiplier(10)->Range(10, 100000);
BENCHMARK(BM_step_list_func)->RangeMultiplier(10)->Range(10, 100000);
BENCHMARK(BM_step_list_iter)->RangeMultiplier(10)->Range(10, 100000);
BENCHMARK(BM_PLFColony_Iteration)->RangeMultiplier(10)->Range(10, 100000);

//BENCHMARK(BM_step_list_iter)->Arg(2048 * 8);
//BENCHMARK(BM_PLFColony_Iteration)->Arg(2048 * 8);

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

// int main()
// {
//     plf::colony<int> i_colony;
//     const int N = 2048 * 32;
//     
//     std::vector<decltype(i_colony.begin())> elms;
//     // Pre-populate the vector with N elements.
//     for (int i = 0; i < N; ++i) {
//         elms.push_back(i_colony.insert(i));
//     }
//     
//     for(int i = 0 ; i < N ; i += 2)
//     {
//         for(auto it = i_colony.begin(); it != i_colony.end() ; it++)
//         {
//             if(it == elms[i])
//             {
//                 i_colony.erase(elms[i]);
//                 break;
//             }
//         }
//     }
//     
//     int_sl sl; int_sl_init(&sl);
//     int **ptrs = (int**) malloc(N * sizeof *ptrs);
//     for (int i = 0; i < N; i++)
//         ptrs[i] = int_sl_put(&sl, i);
//     for (int i = 0; i < N; i += 2)
//         int_sl_pop(&sl, ptrs[i]);
//     
//     bool eq = step_list_equals_vector(&sl, i_colony);
//     if(!eq)
//         puts("NOT EQUAL");
//     
//     free(ptrs);
//     int_sl_deinit(&sl);
// }
