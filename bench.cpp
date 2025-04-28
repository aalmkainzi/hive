#include <list>
#include <iterator>
#include "plf_colony.h"
#include "benchmark/benchmark.h"

extern "C"
{
#define SL_ONLY_DECL
#define SL_TYPE int
#define SL_NAME int_sl

#include "step_list.h"
}

void accumSum(int *elm, void *arg)
{
    *(int*)arg += *elm;
}

#define SL_FOREACH(sl, body) \
for(typeof((sl)->buckets) sl_it_bucket = (sl)->buckets ; sl_it_bucket != NULL ; sl_it_bucket = sl_it_bucket->next) \
for(sl_u sl_it_elms = sl_it_bucket->first_elm_idx ; sl_it_elms < (sizeof(sl_it_bucket->elms) / sizeof(*(sl_it_bucket->elms))) - 1 ; sl_it_elms += sl_it_bucket->jump_list[sl_it_elms]) \
do { \
    body \
    sl_it_elms++; \
} while(0)

#define SL_IT \
((int*const) &sl_it_bucket->elms[sl_it_elms])

static void BM_List_Iteration(benchmark::State& state)
{
    std::list<int> ls;
    const int N = state.range(0);
    
    std::vector<decltype(ls.begin())> elms;
    // Pre-populate the vector with N elements.
    for (int i = 0; i < N; ++i) {
        ls.push_back(i);
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
    
    volatile unsigned int sum = 0;
    for (auto _ : state)
    {
        // Iterate over vector using a range-based for loop.
        for (const auto& value : ls)
        {
            sum += value;
        }
        benchmark::ClobberMemory();
    }
    benchmark::DoNotOptimize(sum);
}

static void BM_step_list(benchmark::State& state) {
    // Perform setup here
    int_sl sl; int_sl_init(&sl);
    const int M = state.range(0);
    int **ptrs = (int**) malloc(M * sizeof *ptrs);
    for (int i = 0; i < M; i++)
        ptrs[i] = int_sl_put(&sl, i);
    for (int i = 0; i < M; i += 2)
        int_sl_pop(&sl, ptrs[i]);
    
    volatile unsigned int sum = 0;
    
    for (auto _ : state) {
        // This code gets timed
        // int_sl_loop(&sl, accumSum, (void*) &sum);
        SL_FOREACH(&sl, sum += *SL_IT;);
    }
    
    free(ptrs);
    int_sl_deinit(&sl);
}

static void BM_PLFColony_Iteration(benchmark::State& state)
{
    plf::colony<int> i_colony;
    const int N = state.range(0);
    
    std::vector<decltype(i_colony.begin())> elms;
    // Pre-populate the vector with N elements.
    for (int i = 0; i < N; ++i) {
        elms.push_back(i_colony.insert(rand() - i));
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
    
    volatile unsigned int sum = 0;
    for (auto _ : state)
    {
        // Iterate over vector using a range-based for loop.
        for (const auto& value : i_colony)
        {
            sum += value;
        }
        benchmark::ClobberMemory();
    }
    benchmark::DoNotOptimize(sum);
}


BENCHMARK(BM_List_Iteration)->Arg(2048 * 32);
BENCHMARK(BM_step_list)->Arg(2048 * 64);
BENCHMARK(BM_PLFColony_Iteration)->Arg(2048 * 64);

BENCHMARK_MAIN();

// Structure to hold comparison context
struct CompareContext {
    const plf::colony<int>& vec;
    bool match = true;
};

// Callback function for int_sl_loop
static void compare_int(int* v, void* arg) {
    CompareContext* ctx = static_cast<CompareContext*>(arg);
    if (std::find(ctx->vec.begin(), ctx->vec.end(), *v) == ctx->vec.end()) {
        ctx->match = false;
    }
}

// Function to compare int_sl and std::vector<int>
bool step_list_equals_vector(const int_sl* sl, const plf::colony<int>& vec) {
    if (sl->count != vec.size()) {
        return false;
    }
    CompareContext ctx{vec};
    int_sl_loop(sl, compare_int, (void*) &ctx);
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
