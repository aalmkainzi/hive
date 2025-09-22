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
            big_sp_deinit(&clone);
            state.ResumeTiming();
        }
            break;
    }
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
            bbig_sp_deinit(&clone);
            state.ResumeTiming();
        }
            break;
    }
}

static void BM_plf(benchmark::State& state)
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
            big_sp_deinit(&clone);
            state.ResumeTiming();
        }
            break;
    }
}

BENCHMARK_MAIN();
