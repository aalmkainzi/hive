#include <cstdint>
#include <iostream>
#include <iterator>
#include <random>
#include "plf_colony.h"

#if !defined(NO_PRINT)
#define printf(...) printf(__VA_ARGS__)
#else
#define printf(...)
#endif
#define rand()      (rand())

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
} Big;

#ifndef TYPE
#define TYPE Big
#endif

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

#define HH_IMPL
#include "../../hetrohive/hetrohive.h"

#define ANKERL_NANOBENCH_IMPLEMENT
#include "nanobench.h"
#include <string>

constexpr std::string_view compiler_name = 
#if defined(__clang__)
"clang";
#else
"gcc";
#endif

int main(int argc, char **argv)
{
    ankerl::nanobench::Bench bench;
    
    std::ofstream outFile(std::string("results/txt/hive_and_plf_colony_").append(compiler_name).append(std::string_view(".txt")));
    bench.output(&outFile);
    
    int begin = 25'000;
    int end   = 550'000;
    int interval = 25'000;
    
    std::string html_file_name = std::string("results/html/res").append(compiler_name).append(".html");
    std::string json_file_name = std::string("results/json/res").append(compiler_name).append(".json");
    
    constexpr bool bench_hive = true;
    constexpr bool bench_small_hive = true;
    constexpr bool bench_plf_colony  = true;
    constexpr bool bench_hetrohive = false;
    
    constexpr bool bench_iter = false;
    constexpr bool bench_put = false;
    constexpr bool bench_pop = false;
    constexpr bool bench_random = false;
    constexpr bool bench_clone = true;
    
    int iterations = 25;
    
    for(int sz = begin ; sz <= end ; sz += interval)
    {
        printf("Starting %d\n", sz);
        std::mt19937 rng(42);
        std::mt19937 rng2(41);
        std::uniform_int_distribution<> dist(0, 1);
        
        if(bench_hive)
        {
            // hive SETUP
            
            srand(69420);
            big_sp sl; big_sp_init(&sl);
            TYPE **ptrs = (TYPE**) malloc(sz * sizeof(ptrs[0]));
            TYPE *bigs = (TYPE*) malloc(sz * sizeof(bigs[0]));
            for (int i = 0; i < sz; i++)
                bigs[i] = (TYPE){i};
            
            big_sp_put_all(&sl, bigs, sz);
            
            int i = 0;
            for(auto it = big_sp_begin(&sl) ; !big_sp_iter_eq(it, big_sp_end(&sl)) ; it = big_sp_iter_next(it))
            {
                ptrs[i++] = it.ptr;
            }
            
            rng.seed(42);
            std::vector<TYPE*> to_pop;
            to_pop.reserve(sz / 2);
            std::sample(ptrs, ptrs + sz, std::back_inserter(to_pop), sz / 2, rng);
            
            for (TYPE *p : to_pop) {
                big_sp_del(&sl, p);
            }
            
            assert(sl.count == sz - sz/2);
            
            // hive END
            
            if(bench_random)
            {
                rng2.seed(41);
                srand(69420);
                
                bench.unit("elms").batch(sz).complexityN(sz).minEpochIterations(iterations).run("hive_rnd",
                    [&]{
                        big_sp_iter it = {};
                        bool iter_set = false;
                        printf("COUNT = %zu\n", sl.count);
                        for(int i = 0, random = rand() % 100 ; i < sz ; i++, random = rand() % 100)
                        {
                            if(random <55 || sl.count == 0 || !iter_set)
                            {
                                big_sp_iter tmp = big_sp_put(&sl, (TYPE){i});
                                
                                random = rand() % 100;
                                if(random < 5 || !iter_set)
                                {
                                    iter_set = true;
                                    it = tmp;
                                }
                            }
                            else
                            {
                                TYPE val = it.ptr[0];
                                it = big_sp_iter_del(&sl, it);
                                if(big_sp_iter_eq(it, big_sp_end(&sl)))
                                    iter_set = false;
                            }
                        }
                        
#if !defined(NDEBUG)
                        unsigned int sum = 0;
                        for(auto it = big_sp_begin(&sl), end = big_sp_end(&sl) ; !big_sp_iter_eq(it,end) ; it = big_sp_iter_next(it))
                        {
                            sum += *it.ptr;
                        }
                        
                        printf("HV SUM AFTER RND: %u\n", sum);
#endif
                        ankerl::nanobench::doNotOptimizeAway(sl);
                    }
                );
            }
            
            if(bench_iter)
            {
                bench.unit("elms").batch(sz).complexityN(sz).minEpochIterations(iterations).run("hive_macro", 
                    [&]{
                        volatile unsigned int sum = 0;
                        // constexpr int HIVE_BUCKET_SIZE = 254;
                        
                        HIVE_FOR_EACH(it, big_sp_begin(&sl), big_sp_end(&sl))
                        {
                            sum += *it.ptr;
                        }
                        
                        ankerl::nanobench::doNotOptimizeAway(sum);
                        printf("hive_macro = %u\n", sum);
                    }
                );
                
                bench.unit("elms").batch(sz).complexityN(sz).minEpochIterations(iterations).run("hive_iter",
                    [&]{
                        volatile unsigned int sum = 0;
                        
                        for(big_sp_iter it = big_sp_begin(&sl), end = big_sp_end(&sl) ; !big_sp_iter_eq(it,end) ; it = big_sp_iter_next(it))
                        {
                            sum += *it.ptr;
                        }
                        
                        ankerl::nanobench::doNotOptimizeAway(sum);
                        printf("hive_iter = %u\n", sum);
                    }
                );
            }
            
            if(bench_pop)
            {
                rng2.seed(41);
                bench.unit("elms").batch(sz).complexityN(sz).minEpochIterations(iterations).run("hive_del",
                    [&]{
                        bool remove = true;
                        big_sp clone = big_sp_clone(&sl);
                        for(big_sp_iter it = big_sp_begin(&clone) ; !big_sp_iter_eq(it, big_sp_end(&clone)) ; )
                        {
                            if(remove)
                                it = big_sp_iter_del(&clone, it);
                            else
                                it = big_sp_iter_next(it);
                            remove = !remove;
                        }
                        
#ifndef NDEBUG
                        unsigned int sum = 0;
                        HIVE_FOR_EACH(it, big_sp_begin(&clone), big_sp_end(&clone))
                        {
                            sum += *it.ptr;
                        }
                        printf("hive_del_sum = %u\n", sum);
#endif
                        
                        big_sp_deinit(&clone);
                        ankerl::nanobench::doNotOptimizeAway(clone);
                    }
                );
            }
            
            if(bench_put)
            {
                rng2.seed(41);
                bench.unit("elms").batch(sz).complexityN(sz).minEpochIterations(iterations).run("hive_put",
                    [&]{
                        big_sp slc = big_sp_clone(&sl);
                        
                        for(int i = 0 ; i < sz/2 ; i++)
                        {
                            auto it = big_sp_put_empty(&slc);
                            *it.ptr = (TYPE){i};
                        }
                        
#if !defined(NDEBUG)
                        unsigned int sum = 0;
                        for(auto it = big_sp_begin(&slc), end = big_sp_end(&slc) ; !big_sp_iter_eq(it,end) ; it = big_sp_iter_next(it))
                        {
                            sum += it.ptr[0];
                        }
                        
                        printf("HV SUM AFTER PUT: %u\n", sum);
#endif
                        ankerl::nanobench::doNotOptimizeAway(slc);
                        
                        big_sp_deinit(&slc);
                    }
                );
            }
            
            if(bench_clone)
            {
                rng2.seed(41);
                bench.unit("elms").batch(sz).complexityN(sz).minEpochIterations(iterations).run("hive_clone",
                    [&]{
                        big_sp slc = big_sp_clone(&sl);
    
#if !defined(NDEBUG)
                        unsigned int sum = 0;
                        for(auto it = big_sp_begin(&slc), end = big_sp_end(&slc) ; !big_sp_iter_eq(it,end) ; it = big_sp_iter_next(it))
                        {
                            sum += it.ptr[0];
                        }
                        
                        printf("HV SUM AFTER CLONE: %u\n", sum);
#endif
                        ankerl::nanobench::doNotOptimizeAway(slc);
                        
                        big_sp_deinit(&slc);
                    }
                );
            
            }
            
            big_sp_deinit(&sl);
            free(ptrs);
            free(bigs);
        }
        
        if(bench_hetrohive)
        {
            // hetrohive SETUP
            
            srand(69420);
            hetrohive sl; hh_init(&sl, 2);
            TYPE **ptrs = (TYPE**) malloc(sz * sizeof(ptrs[0]));
            TYPE *bigs = (TYPE*) malloc(sz * sizeof(bigs[0]));
            for (int i = 0; i < sz; i++)
                bigs[i] = (TYPE){i};
            
            hh_register_type(&sl, 1, sizeof(TYPE));
            hh_put_all(&sl, bigs, sz, 1);
            
            int i = 0;
            
            for(auto it = hh_begin(&sl); !hh_iter_eq(it, hh_end(&sl)) ; it = hh_iter_next(it))
            {
                ptrs[i++] = (TYPE*) hh_iter_elm(it);
            }
            
            rng.seed(42);
            std::vector<TYPE *> to_pop;
            to_pop.reserve(sz / 2);
            std::sample(ptrs, ptrs + sz, std::back_inserter(to_pop), sz / 2, rng);
            
            for (TYPE *p : to_pop) {
                hh_del(&sl, p);
            }
            
            assert(sl.count == sz - sz/2);
            
            // hetrohive END
            
            if(bench_random)
            {
                rng2.seed(41);
                srand(69420);
                
                bench.unit("elms").batch(sz).complexityN(sz).minEpochIterations(iterations).run("hh_rnd",
                    [&]{
                        hh_iter it = {};
                        bool iter_set = false;
                        printf("COUNT = %zu\n", sl.count);
                        for(int i = 0, random = rand() % 100 ; i < sz ; i++, random = rand() % 100)
                        {
                            if(random <55 || sl.count == 0 || !iter_set)
                            {
                                TYPE elm = {i};
                                hh_iter tmp = hh_put(&sl, &elm, 1);
                                
                                random = rand() % 100;
                                if(random < 5 || !iter_set)
                                {
                                    iter_set = true;
                                    it = tmp;
                                }
                            }
                            else
                            {
                                int val = *((TYPE*)hh_iter_elm(it));
                                it = hh_iter_del(&sl, it);
                                if(hh_iter_eq(it, hh_end(&sl)))
                                    iter_set = false;
                            }
                        }
                        
#if !defined(NDEBUG)
                        unsigned int sum = 0;
                        for(auto it = hh_begin(&sl), end = hh_end(&sl) ; !hh_iter_eq(it,end) ; it = hh_iter_next(it))
                        {
                            sum += *((TYPE*)hh_iter_elm(it));
                        }
                        
                        printf("HH SUM AFTER RND: %u\n", sum);
#endif
                        ankerl::nanobench::doNotOptimizeAway(sl);
                    }
                );
            }
            
            if(bench_iter)
            {
                bench.unit("elms").batch(sz).complexityN(sz).minEpochIterations(iterations).run("hh_iter",
                    [&]{
                        volatile unsigned int sum = 0;
                        
                        for(hh_iter it = hh_begin(&sl), end = hh_end(&sl) ; !hh_iter_eq(it,end) ; it = hh_iter_next(it))
                        {
                            sum += *((TYPE*)hh_iter_elm(it));
                        }
                        
                        ankerl::nanobench::doNotOptimizeAway(sum);
                        printf("hh_iter = %u\n", sum);
                    }
                );
                
                bench.unit("elms").batch(sz).complexityN(sz).minEpochIterations(iterations).run("hh_typed_iter",
                    [&]{
                        volatile unsigned int sum = 0;
                        
                        for(hh_typed_iter it = hh_typed_begin(&sl, 1), end = hh_typed_end(&sl, 1) ; !hh_typed_iter_eq(it,end) ; it = hh_typed_iter_next(it))
                        {
                            sum += *((TYPE*)hh_typed_iter_elm(it));
                        }
                        
                        ankerl::nanobench::doNotOptimizeAway(sum);
                        printf("hh_typed_iter = %u\n", sum);
                    }
                );
            }
            
            if(bench_pop)
            {
                rng2.seed(41);
                bench.unit("elms").batch(sz).complexityN(sz).minEpochIterations(iterations).run("hh_del",
                    [&]{
                        hh_iter_del(&sl, hh_begin(&sl));
                        
                        ankerl::nanobench::doNotOptimizeAway(sl);
                    }
                );
            }
            
            if(bench_put)
            {
                rng2.seed(41);
                bench.unit("elms").batch(sz).complexityN(sz).minEpochIterations(iterations).run("hh_put",
                    [&]{
                        hetrohive slc = sl;//hh_clone(&sl);
                        
                        for(int i = 0 ; i < sz/2 ; i++)
                        {
                            TYPE elm = {i};
                            hh_put(&slc, &elm, 1);
                        }
                        
#if !defined(NDEBUG)
                        unsigned int sum = 0;
                        for(auto it = hh_begin(&slc), end = hh_end(&slc) ; !hh_iter_eq(it,end) ; it = hh_iter_next(it))
                        {
                            sum += *((TYPE*)hh_iter_elm(it));
                        }
                        
                        printf("HH SUM AFTER PUT: %u\n", sum);
#endif
                        hh_deinit(&slc);
                        ankerl::nanobench::doNotOptimizeAway(slc);
                    }
                );
            }
            
            hh_deinit(&sl);
            free(ptrs);
            free(bigs);
        }
        
        if(bench_small_hive)
        {
            // hive SETUP
            
            srand(69420);
            bbig_sp sl; bbig_sp_init(&sl);
            TYPE **ptrs = (TYPE**) malloc(sz * sizeof(ptrs[0]));
            TYPE *bigs =  (TYPE*) malloc(sz * sizeof(bigs[0]));
            for (int i = 0; i < sz; i++)
                bigs[i] = (TYPE){i};
            
            bbig_sp_put_all(&sl, bigs, sz);
            
            int i = 0;
            
            HIVE_FOR_EACH_(it, bbig_sp_begin(&sl), bbig_sp_end(&sl))
            {
                ptrs[i++] = it.elm;
            }
            
            rng.seed(42);
            std::vector<TYPE*> to_pop;
            to_pop.reserve(sz / 2);
            std::sample(ptrs, ptrs + sz,
                        std::back_inserter(to_pop),
                        sz / 2,
                        rng);
            
            for (TYPE* p : to_pop) {
                bbig_sp_del(&sl, p);
            }
            
            assert(sl.count == sz - sz/2);
            
            // STABLE_POOL END
            if(bench_random)
            {
                rng2.seed(41);
                srand(69420);
                
                bench.unit("elms").batch(sz).complexityN(sz).minEpochIterations(iterations).run("bhive_rnd",
                    [&]{
                        bbig_sp_iter it = {};
                        bool iter_set = false;
                        printf("COUNT = %zu\n", sl.count);
                        for(int i = 0, random = rand() % 100 ; i < sz ; i++, random = rand() % 100)
                        {
                            if(random <55 || sl.count == 0 || !iter_set)
                            {
                                bbig_sp_iter tmp = bbig_sp_put(&sl, (TYPE){i});
                                
                                random = rand() % 100;
                                if(random < 5 || !iter_set)
                                {
                                    iter_set = true;
                                    it = tmp;
                                }
                            }
                            else
                            {
                                int val = it.elm[0];
                                it = bbig_sp_iter_del(&sl, it);
                                if(bbig_sp_iter_eq(it, bbig_sp_end(&sl)))
                                    iter_set = false;
                            }
                        }
                        
#if !defined(NDEBUG)
                        unsigned int sum = 0;
                        for(auto it = bbig_sp_begin(&sl), end = bbig_sp_end(&sl) ; !bbig_sp_iter_eq(it,end) ; it = bbig_sp_iter_next(it))
                        {
                            sum += bbig_sp_iter_elm(it)[0];
                        }
                        
                        printf("HV SUM AFTER RND: %u\n", sum);
#endif
                        ankerl::nanobench::doNotOptimizeAway(sl);
                    }
                );
            }
            
            if(bench_iter)
            {
                bench.unit("elms").batch(sz).complexityN(sz).minEpochIterations(iterations).run("old_hive_iter",
                [&]{
                    volatile unsigned int sum = 0;
                    
                    const bbig_sp_bucket_t *const end = sl.end_sentinel;
                    for(bbig_sp_iter it = bbig_sp_begin(&sl) ; it.bucket != end ; it = bbig_sp_iter_next(it))
                    {
                        sum += bbig_sp_iter_elm(it)[0];
                    }
                    
                    ankerl::nanobench::doNotOptimizeAway(sum);
                    printf("old_hive_iter = %u\n", sum);
                }
                );
                bench.unit("elms").batch(sz).complexityN(sz).minEpochIterations(iterations).run("old_hive_macro",
                [&]{
                    volatile unsigned int sum = 0;
                    
                    const bbig_sp_bucket_t *const end = sl.end_sentinel;
                    
                    HIVE_FOR_EACH_(it, bbig_sp_begin(&sl), bbig_sp_end(&sl))
                    {
                        sum += it.elm[0];
                    }
                    
                    ankerl::nanobench::doNotOptimizeAway(sum);
                    printf("old_hive_macro = %u\n", sum);
                }
                );
            }
            
            if(bench_put)
            {
                rng2.seed(41);
                bench.unit("elms").batch(sz).complexityN(sz).minEpochIterations(iterations).run("bhive_put",
                    [&]{
                        bbig_sp slc = bbig_sp_clone(&sl);
                        
                        for(int i = 0 ; i < sz/2 ; i++)
                        {
                            auto it = bbig_sp_put_empty(&slc);
                            *it.elm = (TYPE){i};
                        }
                        
#ifndef NDEBUG
                        unsigned int sum = 0;
                        for(auto it = bbig_sp_begin(&slc) ; !bbig_sp_iter_eq(it, bbig_sp_end(&slc)) ; it = bbig_sp_iter_next(it))
                        {
                            sum += bbig_sp_iter_elm(it)[0];
                        }
#endif
                        printf("SP SUM AFTER PUT: %u\n", sum);
                        ankerl::nanobench::doNotOptimizeAway(slc);
                        bbig_sp_deinit(&slc);
                    }
                );
            }
            if(bench_clone)
            {
                rng2.seed(41);
                bench.unit("elms").batch(sz).complexityN(sz).minEpochIterations(iterations).run("bhive_clone",
                    [&]{
                        bbig_sp slc = bbig_sp_clone(&sl);
                        
#ifndef NDEBUG
                        unsigned int sum = 0;
                        for(auto it = bbig_sp_begin(&slc) ; !bbig_sp_iter_eq(it, bbig_sp_end(&slc)) ; it = bbig_sp_iter_next(it))
                        {
                            sum += bbig_sp_iter_elm(it)[0];
                        }
#endif
                        printf("SP SUM AFTER CLONE: %u\n", sum);
                        ankerl::nanobench::doNotOptimizeAway(slc);
                        bbig_sp_deinit(&slc);
                    }
                );
            }
            if(bench_pop)
            {
                rng2.seed(41);
                bench.unit("elms").batch(sz).complexityN(sz).minEpochIterations(iterations).run("bhive_del",
                    [&]{
                        bool remove = true;
                        bbig_sp clone = bbig_sp_clone(&sl);
                        for(bbig_sp_iter it = bbig_sp_begin(&clone) ; !bbig_sp_iter_eq(it, bbig_sp_end(&clone)) ; )
                        {
                            if(remove)
                                it = bbig_sp_iter_del(&clone, it);
                            else
                                it = bbig_sp_iter_next(it);
                            remove = !remove;
                        }
                        
#ifndef NDEBUG
                        unsigned int sum = 0;
                        HIVE_FOR_EACH_(it, bbig_sp_begin(&clone), bbig_sp_end(&clone))
                        {
                            sum += it.elm[0];
                        }
                        printf("old_del_sum = %u\n", sum);
#endif
                        
                        bbig_sp_deinit(&clone);
                        ankerl::nanobench::doNotOptimizeAway(clone);
                    }
                );
            }
            
            
            bbig_sp_deinit(&sl);
            free(ptrs);
            free(bigs);
        }
        
        if(bench_plf_colony)
        {
            // PLF SETUP
            srand(69420);
            plf::colony<TYPE> i_colony;
            
            std::vector<decltype(i_colony.begin())> elms;
            elms.reserve(sz);
            for (int i = 0; i < sz; ++i)
            {
                elms.push_back(i_colony.insert((TYPE){i}));
            }
            
            rng.seed(42);
            std::vector<decltype(i_colony.begin())> to_erase;
            to_erase.reserve(sz/2);
            std::sample(elms.begin(), elms.end(),
                        std::back_inserter(to_erase),
                        sz/2,
                        rng);
            
            auto beg = to_erase.begin();
            auto end = to_erase.end();
            for ( ; beg != end ; beg++)
            {
                i_colony.erase(*beg);
            }
            
            assert(i_colony.size() == sz - sz/2);
            // PLF SETUP END
            if(bench_random)
            {
                rng2.seed(41);
                srand(69420);
                bench.unit("elms").batch(sz).complexityN(sz).minEpochIterations(iterations).run("plf_colony_rnd",
                    [&]{
                        bool set_iter = false;
                        decltype(i_colony)::iterator it = i_colony.begin();
                        for(int i = 0, random = rand() % 100 ; i < sz ; i++, random = rand() % 100)
                        {
                            if(random < 55 || i_colony.size() == 0 || !set_iter)
                            {
                                auto tmp = i_colony.insert((TYPE){i});
                                random = rand() % 100;
                                if(random < 5 || !set_iter)
                                {
                                    set_iter = true;
                                    it = tmp;
                                }
                            }
                            else
                            {
                                it = i_colony.erase(it);
                                if(it == i_colony.end())
                                    set_iter = false;
                            }
                        }
                        
#if !defined(NDEBUG)
                        unsigned int sum = 0;
                        for(auto it = i_colony.begin() ; it != i_colony.end() ; it++)
                        {
                            sum += *it;
                        }
                        
                        printf("PLF SUM AFTER RND: %u\n", sum);
#endif
                        ankerl::nanobench::doNotOptimizeAway(i_colony);
                    }
                );
            }
            if(bench_iter)
                bench.unit("elms").batch(sz).complexityN(sz).minEpochIterations(iterations).run("plf::colony", 
                    [&]{
                        volatile unsigned int sum = 0;
                        
                        for (const auto& value : i_colony)
                        {
                            sum += (int)value;
                        }
                        ankerl::nanobench::doNotOptimizeAway(sum);
                        printf("plfsum = %u\n", sum);
                        
                    }
                );
            
            if(bench_pop)
            {
                // std::cout << "SZ: " << sz << std::endl;
                rng2.seed(41);
                bench.unit("elms").batch(sz).complexityN(sz).minEpochIterations(iterations).run("plf::colony::erase", 
                [&]{
                    plf::colony<TYPE> icol = plf::colony<TYPE>(i_colony);
                    bool remove = true;
                    for (plf::colony<TYPE>::iterator it = icol.begin() ; it != icol.end() ; )
                    {
                        if(remove)
                            it = icol.erase(it);
                        else
                            it++;
                        remove = !remove;
                    }
                    
#ifndef NDEBUG
                    unsigned int sum = 0;
                    for(auto it : icol)
                    {
                        sum += it;
                    }
                    printf("plf_del_sum=%u\n", sum);
#endif
                    
                    ankerl::nanobench::doNotOptimizeAway(icol);
                }
                );
            }
            
            if(bench_put)
            {
                // std::cout << "SZ: " << sz << std::endl;
                rng2.seed(41);
                bench.unit("elms").batch(sz).complexityN(sz).minEpochIterations(iterations).run("plf::colony::insert", 
                [&]{
                    plf::colony<TYPE> icol = plf::colony<TYPE>(i_colony);
                    
                    for (int i = 0 ; i < sz/2 ; i++)
                    {
                        icol.insert((TYPE){i});
                    }
#if !defined(NDEBUG)
                    unsigned int sum = 0;
                    for(auto it : icol)
                    {
                        sum += it;
                    }
                    
                    printf("PLF SUM AFTER INS %u\n", sum);
#endif
                    ankerl::nanobench::doNotOptimizeAway(icol);
                }
                );
            }
            if(bench_clone)
            {
                // std::cout << "SZ: " << sz << std::endl;
                rng2.seed(41);
                bench.unit("elms").batch(sz).complexityN(sz).minEpochIterations(iterations).run("plf::colony::insert", 
                [&]{
                    plf::colony<TYPE> icol = plf::colony<TYPE>(i_colony);
                    
#if !defined(NDEBUG)
                    unsigned int sum = 0;
                    for(auto it : icol)
                    {
                        sum += it;
                    }
                    
                    printf("PLF SUM AFTER INS %u\n", sum);
#endif
                    ankerl::nanobench::doNotOptimizeAway(icol);
                }
                );
            }
        }
        
        fprintf(stdout, "Done from %d\n", sz);
    }
    std::ofstream htmlFile(html_file_name);
    std::ofstream jsonFile(json_file_name);
    
    bench.render(ankerl::nanobench::templates::htmlBoxplot(), htmlFile);
    bench.render(ankerl::nanobench::templates::json(), jsonFile);
    
    return 0;
}
