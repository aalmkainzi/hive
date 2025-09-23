#include <cstdint>
#include <iostream>
#include <iterator>
#include <random>
#include "plf_colony.h"
#include "benchmark/include/benchmark/benchmark.h"

#if !defined(NO_PRINT)
#define printf(...) printf(__VA_ARGS__)
#else
#define printf(...)
#endif

#ifndef TYPE
#define TYPE Big
#endif

typedef struct Big
{
    int i;
    char _m[256 - 4];

    bool operator==(const Big& other) const {
        return this->i == other.i;
    }
    
    operator int() const
    {
        return i;
    }
    
    Big& operator=(int ii)
    {
        i = ii;
        return *this;
    }
} Big;

#define HIVE_IMPL
#define HIVE_TYPE TYPE
#define HIVE_NAME big_sp
#include "hive.h"

#define HIVE_IMPL
#define HIVE_TYPE TYPE
#define HIVE_NAME bbig_sp
#if __has_include("../../hive_old/hive.h")
#include "../../hive_old/hive.h"
#else
#include "hive.h"
#endif

constexpr std::string_view compiler_name = 
#if defined(__clang__)
"clang";
#else
"gcc";
#endif

enum BenchType {
    PUT, POP, ITER
};

constexpr BenchType bench_type = PUT;

void hive_print_sum(big_sp *sp)
{
#if !defined(NDEBUG)
    unsigned int sum = 0;
    HIVE_FOR_EACH(it, big_sp_begin(sp), big_sp_end(sp))
    {
        sum += *it.ptr;
    }
#endif
}

void bhive_print_sum(bbig_sp *sp)
{
#if !defined(NDEBUG)
    unsigned int sum = 0;
    HIVE_FOR_EACH_(it, bbig_sp_begin(sp), bbig_sp_end(sp))
    {
        sum += *it.elm;
    }
#endif
}

static void BM_hive(benchmark::State& state)
{
    const int N = state.range(0);
    std::mt19937 rng(42);
    
    TYPE **ptrs = (TYPE**) malloc(N * sizeof(ptrs[0]));
    
    big_sp sp;
    big_sp_init(&sp);
    
    for(int i = 0 ; i < N ; i++)
    {
        auto it = big_sp_put_empty(&sp);
        *it.ptr = i;
        ptrs[i] = it.ptr;
    }
    
    std::vector<TYPE*> to_pop;
    to_pop.reserve(N / 2);
    std::sample(ptrs, ptrs + N, std::back_inserter(to_pop), N / 2, rng);
    
    for (TYPE *p : to_pop) {
        big_sp_del(&sp, p);
    }
    
    for(auto _ : state)
    {
        switch(bench_type)
        {
            case ITER:
            {
                unsigned int sum = 0;
                for(big_sp_iter it = big_sp_begin(&sp),
                    end = big_sp_end(&sp) ;
                !big_sp_iter_eq(it, end) ;
                it = big_sp_iter_next(it))
                {
                    sum += *it.ptr;
                }
                hive_print_sum(&sp);
                benchmark::DoNotOptimize(sum);
            }
            break;
            case PUT:
            {
                state.PauseTiming();
                big_sp clone = big_sp_clone(&sp);
                state.ResumeTiming();
                for(int i = 0 ; i < N / 2 ; i++)
                {
                    auto it = big_sp_put_empty(&clone);
                    *it.ptr = i;
                }
                benchmark::DoNotOptimize(clone);
                state.PauseTiming();
                hive_print_sum(&clone);
                big_sp_deinit(&clone);
                state.ResumeTiming();
            }
            break;
            case POP:
            {
                state.PauseTiming();
                big_sp clone = big_sp_clone(&sp);
                state.ResumeTiming();
                bool remove = true;
                for(big_sp_iter it = big_sp_begin(&sp) ; !big_sp_iter_eq(it, big_sp_end(&sp)) ; )
                {
                    if(remove)
                    {
                        it = big_sp_iter_del(&sp, it);
                    }
                    else
                    {
                        it = big_sp_iter_next(it);
                    }
                }
                benchmark::DoNotOptimize(clone);
                state.PauseTiming();
                hive_print_sum(&clone);
                big_sp_deinit(&clone);
                state.ResumeTiming();
            }
            break;
        }
    }
    big_sp_deinit(&sp);
    free(ptrs);
}

static void BM_bhive(benchmark::State& state)
{
    const int N = state.range(0);
    std::mt19937 rng(42);
    
    TYPE **ptrs = (TYPE**) malloc(N * sizeof(ptrs[0]));
    
    bbig_sp sp;
    bbig_sp_init(&sp);
    
    for(int i = 0 ; i < N ; i++)
    {
        auto it = bbig_sp_put(&sp, TYPE{i});
        ptrs[i] = it.elm;
    }
    
    std::vector<TYPE*> to_pop;
    to_pop.reserve(N / 2);
    std::sample(ptrs, ptrs + N, std::back_inserter(to_pop), N / 2, rng);
    
    for (TYPE *p : to_pop) {
        bbig_sp_del(&sp, p);
    }
    
    for(auto _ : state)
    {
        switch(bench_type)
        {
            case ITER:
            {
                unsigned int sum = 0;
                for(bbig_sp_iter it = bbig_sp_begin(&sp),
                    end = bbig_sp_end(&sp) ;
                !bbig_sp_iter_eq(it, end) ;
                it = bbig_sp_iter_next(it))
                {
                    sum += *it.elm;
                }
                bhive_print_sum(&sp);
                benchmark::DoNotOptimize(sum);
            }
            break;
            case PUT:
            {
                state.PauseTiming();
                bbig_sp clone = bbig_sp_clone(&sp);
                state.ResumeTiming();
                for(int i = 0 ; i < N / 2 ; i++)
                {
                    auto it = bbig_sp_put(&clone, TYPE{i});
                }
                benchmark::DoNotOptimize(clone);
                state.PauseTiming();
                bhive_print_sum(&clone);
                bbig_sp_deinit(&clone);
                state.ResumeTiming();
            }
            break;
            case POP:
            {
                state.PauseTiming();
                bbig_sp clone = bbig_sp_clone(&sp);
                state.ResumeTiming();
                bool remove = true;
                for(bbig_sp_iter it = bbig_sp_begin(&sp) ; !bbig_sp_iter_eq(it, bbig_sp_end(&sp)) ; )
                {
                    if(remove)
                    {
                        it = bbig_sp_iter_del(&sp, it);
                    }
                    else
                    {
                        it = bbig_sp_iter_next(it);
                    }
                }
                benchmark::DoNotOptimize(clone);
                state.PauseTiming();
                bhive_print_sum(&clone);
                bbig_sp_deinit(&clone);
                state.ResumeTiming();
            }
            break;
        }
    }
    
    bbig_sp_deinit(&sp);
    free(ptrs);
}

void plf_print_sum(plf::colony<TYPE> *plf)
{
#if !defined(NDEBUG)
    unsigned int sum = 0;
    for(auto it : *plf)
    {
        sum += it;
    }
    
    printf("PLF SUM = %u\n", sum);
#endif
}

static void BM_plf(benchmark::State& state)
{
    const int N = state.range(0);
    std::mt19937 rng(42);
    plf::colony<TYPE> col;
    
    decltype(col.begin()) *ptrs = (decltype(col.begin())*) malloc(N * sizeof(ptrs[0]));
    
    for(int i = 0 ; i < N ; i++)
    {
        auto it = col.insert(TYPE{i});
        ptrs[i] = it;
    }
    
    std::vector<decltype(col.begin())> to_pop;
    to_pop.reserve(N / 2);
    std::sample(ptrs, ptrs + N, std::back_inserter(to_pop), N / 2, rng);
    
    for (auto p : to_pop) {
        col.erase(p);
    }
    
    for(auto _ : state)
    {
        switch(bench_type)
        {
            case ITER:
            {
                unsigned int sum = 0;
                for(auto it : col)
                {
                    sum += it;
                }
#if !defined(NDEBUG)
                plf_print_sum(&col);
#endif
                benchmark::DoNotOptimize(sum);
            }
            break;
            case PUT:
            {
                state.PauseTiming();
                plf::colony<TYPE> *clone = new plf::colony<TYPE>(col);
                state.ResumeTiming();
                for(int i = 0 ; i < N / 2 ; i++)
                {
                    auto it = clone->insert(TYPE{i});
                }
                benchmark::DoNotOptimize(clone);
                state.PauseTiming();
                plf_print_sum(clone);
                delete clone;
                state.ResumeTiming();
            }
            break;
            case POP:
            {
                state.PauseTiming();
                plf::colony<TYPE> *clone = new plf::colony<TYPE>(col);
                state.ResumeTiming();
                bool remove = true;
                for(auto it = clone->begin() ; it != clone->end() ; )
                {
                    if(remove)
                    {
                        it = clone->erase(it);
                    }
                    else
                    {
                        it++;
                    }
                }
                benchmark::DoNotOptimize(clone);
                state.PauseTiming();
                plf_print_sum(clone);
                delete clone;
                state.ResumeTiming();
            }
            break;
        }
    }
}

BENCHMARK(BM_hive) ->Arg(1000)->Arg(10000)->Arg(100000)->Arg(500000);
BENCHMARK(BM_bhive)->Arg(1000)->Arg(10000)->Arg(100000)->Arg(500000);
BENCHMARK(BM_plf)  ->Arg(1000)->Arg(10000)->Arg(100000)->Arg(500000);

BENCHMARK_MAIN();
