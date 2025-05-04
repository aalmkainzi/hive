#include <list>
#include <iterator>
#include "plf_colony.h"
#include "benchmark/benchmark.h"

typedef struct Big
{
    char _m[256 - 4];
    int i;

    bool operator==(const Big& other) const {
        return this->i == other.i;
    }
} Big;

extern "C"
{
// #define SL_ONLY_DECL
#define SL_TYPE Big
#define SL_NAME big_sl

#include "step_list.h"
}

void accumSum(Big *elm, void *arg)
{
    *(int*)arg += elm->i;
}

static void BM_List_Iteration(benchmark::State& state)
{
    std::list<Big> ls;
    const int N = state.range(0);
    
    std::vector<decltype(ls.begin())> elms;
    // Pre-populate the vector with N elements.
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
        // Iterate over vector using a range-based for loop.
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
    big_sl sl; big_sl_init(&sl);
    const int M = state.range(0);
    Big **ptrs = (Big**) malloc(M * sizeof(ptrs[0]));
    for (int i = 0; i < M; i++)
        ptrs[i] = big_sl_put(&sl, (Big){.i = i});
    for (int i = 0; i < M; i += 2)
        big_sl_pop(&sl, ptrs[i]);
    
    volatile unsigned int sum = 0;
    big_sl *ssl = &sl;
    // printf("sl size = %zu\n", sl.count);
    for (auto _ : state) {
        // This code gets timed
        // big_sl_loop(&sl, accumSum, (void*) &sum);
        
        SL_FOREACH(ssl, sum += SL_IT->i; );
        
        benchmark::ClobberMemory();
        benchmark::DoNotOptimize(sum);
    }
    
    free(ptrs);
    big_sl_deinit(&sl);
}

static void BM_PLFColony_Iteration(benchmark::State& state)
{
    plf::colony<Big> i_colony;
    const int N = state.range(0);
    
    std::vector<decltype(i_colony.begin())> elms;
    // Pre-populate the vector with N elements.
    for (int i = 0; i < N; ++i) {
        elms.push_back(i_colony.insert((Big){.i=rand() - i}));
    }
    
    for(int i = 0 ; i < N ; i += 2)
    {
        for(auto it = i_colony.begin(); it != i_colony.end() ; it++)
        {
            if(it == elms[i])
            {
                i_colony.erase(elms[i]);
                break;
            }
        }
    }
    
    // printf("std::list size = %zu\n", i_colony.size());
    volatile unsigned int sum = 0;
    for (auto _ : state)
    {
        // Iterate over vector using a range-based for loop.
        for (const auto& value : i_colony)
        {
            sum += value.i;
        }
        benchmark::ClobberMemory();
        benchmark::DoNotOptimize(sum);
    }
}

BENCHMARK(BM_List_Iteration)->Arg(2048 * 32);
BENCHMARK(BM_step_list)->Arg(2048 * 32);
BENCHMARK(BM_PLFColony_Iteration)->Arg(2048 * 32);

BENCHMARK_MAIN();

// Structure to hold comparison context
struct CompareContext {
    const plf::colony<Big>& vec;
    bool match = true;
};

// Callback function for int_sl_loop
static void compare_int(Big* v, void* arg) {
    CompareContext* ctx = static_cast<CompareContext*>(arg);
    if (std::find(ctx->vec.begin(), ctx->vec.end(), *v) == ctx->vec.end()) {
        ctx->match = false;
    }
}

// Function to compare int_sl and std::vector<int>
bool step_list_equals_vector(const big_sl* sl, const plf::colony<Big>& vec) {
    if (sl->count != vec.size()) {
        return false;
    }
    CompareContext ctx{vec};
    big_sl_loop(sl, compare_int, (void*) &ctx);
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
