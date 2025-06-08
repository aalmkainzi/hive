#include <cstdint>
#include <iostream>
#include <list>
#include <forward_list>
#include <iterator>
#include <random>
#include <boost/container/stable_vector.hpp>
#include "plf_colony.h"
#include "plf_list.h"
#include "slot_map.h"

#if !defined(NO_PRINT)
#define printf(...) printf(__VA_ARGS__)
#else
#define printf(...)
#endif
#define rand()      (rand())

typedef struct Big
{
    char _m[256 - 4];
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

#define HIVE_IMPL
#define HIVE_TYPE Big
#define HIVE_NAME big_sp
#include "hive.h"

#define HIVE_IMPL
#define HIVE_TYPE Big
#define HIVE_NAME bbig_sp
#if __has_include("../../sp_other/hive.h")
#include "../../sp_other/hive.h"
#else
#include "hive.h"
#endif

#define BRACE_TYPE Big
#define BRACE_NAME bbrace
#include "../../brace/brace.h"

#define HH_IMPL
#include "../../hetrohive/hetrohive.h"

void add_sum(Big *big, void *arg)
{
    *(unsigned int*)arg += big->i;
}

#define ANKERL_NANOBENCH_IMPLEMENT
#include "nanobench.h"
#include <string>

constexpr std::string_view compiler_name = 
#if defined(__clang__)
"clang";
#else
"gcc";
#endif

unsigned int hv_sum(big_sp sl)
{
    unsigned int sum = 0;
    HIVE_FOREACH(&sl,
                 sum += HIVE_ITER_ELM->i;);
    return sum;
}

void print_hv(big_sp sl, char *fname)
{
    FILE *f;
    size_t bucket_size;
    
    f = fopen(fname, "w");
    bucket_size = HIVE_GET_BUCKET_SIZE(&sl);
    
    fwrite(&sl.count, sizeof(sl.count), 1, f);
    fwrite(&sl.bucket_count, sizeof(sl.bucket_count), 1, f);
    fwrite(&bucket_size, sizeof(bucket_size), 1, f);
    
    typedef struct bucket_data
    {
        uint16_t not_full_idx; // if this bucket is not full, this will be set to its index inside the `not_full_buckets` array
        uint16_t first_empty_idx;
        uint16_t first_elm_idx;
        uint16_t count;
        big_sp_entry_t elms[HIVE_GET_BUCKET_SIZE(&sl) + 1];
        big_sp_next_entry_t next_entries[HIVE_GET_BUCKET_SIZE(&sl) + 1];
    } bucket_data;
    
    big_sp_bucket_t *b = sl.buckets;
    while(b != NULL)
    {
        bucket_data bd = {
            .not_full_idx = b->not_full_idx,
            .first_empty_idx = b->first_empty_idx,
            .first_elm_idx = b->first_elm_idx,
            .count = b->count,
            // .elms = b->elms,
            // .next_entries = b->next_entries
        };
        memcpy(bd.elms, b->elms, sizeof(bd.elms));
        memcpy(bd.next_entries, b->next_entries, sizeof(bd.next_entries));
        
        fwrite(&bd, sizeof(bd), 1, f);
        b = b->next;
    }
}

int main(int argc, char **argv)
{
    ankerl::nanobench::Bench bench;
    
    std::ofstream outFile(std::string("results/txt/hive_and_plf_colony_").append(compiler_name).append(std::string_view(".txt")));
    bench.output(&outFile);
    
    int begin = 100;
    int end   = 150'000;
    int interval = 25'000;
    
    std::string html_file_name = std::string("results/html/hive_and_plf_colony_").append(compiler_name).append(".html");
    std::string json_file_name = std::string("results/json/hive_and_plf_colony_").append(compiler_name).append(".json");
    
    constexpr bool bench_hive = true;
    constexpr bool bench_small_hive = false;
    constexpr bool bench_plf_colony  = true;
    constexpr bool bench_slot_map    = false;
    constexpr bool bench_stable_vec  = false;
    constexpr bool bench_linked_list = false;
    constexpr bool bench_brace = false;
    constexpr bool bench_hetrohive = true;
    
    constexpr bool bench_iter = true;
    constexpr bool bench_put = false;
    constexpr bool bench_pop = false;
    constexpr bool bench_random = false;
    
    int iterations = 1;
    
    for(int sz = begin ; sz <= end ; sz += interval)
    {
        printf("Starting %d\n", sz);
        std::mt19937 rng(42);
        std::mt19937 rng2(41);
        std::uniform_int_distribution<> dist(0, 1);
        
        if(bench_hive)
        {
            // STABLE_POOL SETUP
            
            srand(69420);
            big_sp sl; big_sp_init(&sl);
            Big **ptrs = (Big**) malloc(sz * sizeof(ptrs[0]));
            Big *bigs = (Big*) malloc(sz * sizeof(bigs[0]));
            for (int i = 0; i < sz; i++)
                bigs[i] = (Big){.i = i};
            
            big_sp_put_all(&sl, bigs, sz);
            
            int i = 0;
            
            HIVE_FOREACH(
                &sl,
                ptrs[i++] = HIVE_ITER_ELM;
            );
            
            rng.seed(42);
            std::vector<Big *> to_pop;
            to_pop.reserve(sz / 2);
            std::sample(ptrs, ptrs + sz, std::back_inserter(to_pop), sz / 2, rng);
            
            for (Big *p : to_pop) {
                big_sp_del(&sl, p);
            }
            
            assert(sl.count == sz - sz/2);
            
            // STABLE_POOL END
            
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
                                big_sp_iter tmp = big_sp_put(&sl, (Big){.i=i});
                                
                                random = rand() % 100;
                                if(random < 5 || !iter_set)
                                {
                                    iter_set = true;
                                    it = tmp;
                                }
                            }
                            else
                            {
                                int val = it.elm->value.i;
                                it = big_sp_iter_del(&sl, it);
                                if(big_sp_iter_eq(it, big_sp_end(&sl)))
                                    iter_set = false;
                            }
                        }
                        
#if !defined(NDEBUG)
                        unsigned int sum = 0;
                        for(auto it = big_sp_begin(&sl), end = big_sp_end(&sl) ; !big_sp_iter_eq(it,end) ; big_sp_iter_go_next(&it))
                        {
                            sum += big_sp_iter_elm(it)->i;
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
                        
                        HIVE_FOREACH(&sl, sum += HIVE_ITER_ELM->i; );
                        
                        ankerl::nanobench::doNotOptimizeAway(sum);
                        printf("hive = %u\n", sum);
                    }
                );
                
                bench.unit("elms").batch(sz).complexityN(sz).minEpochIterations(iterations).run("hive_iter",
                    [&]{
                        volatile unsigned int sum = 0;
                        
                        for(big_sp_iter it = big_sp_begin(&sl), end = big_sp_end(&sl) ; !big_sp_iter_eq(it,end) ; big_sp_iter_go_next(&it))
                        {
                            sum += big_sp_iter_elm(it)->i;
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
                        big_sp_iter_del(&sl, big_sp_begin(&sl));
                        
                        ankerl::nanobench::doNotOptimizeAway(sl);
                    }
                );
            }
            
            if(bench_put)
            {
                rng2.seed(41);
                bench.unit("elms").batch(sz).complexityN(sz).minEpochIterations(iterations).run("hive_put",
                    [&]{
                        // big_sp slc = big_sp_clone(&sl);
                        
                        for(int i = 0 ; i < sz/2 ; i++)
                        {
                            big_sp_put(&sl, (Big){.i=i});
                        }
                        
#if !defined(NDEBUG)
                        unsigned int sum = 0;
                        for(auto it = big_sp_begin(&sl), end = big_sp_end(&sl) ; !big_sp_iter_eq(it,end) ; big_sp_iter_go_next(&it))
                        {
                            sum += big_sp_iter_elm(it)->i;
                        }
                        
                        printf("HV SUM AFTER PUT: %u\n", sum);
#endif
                        ankerl::nanobench::doNotOptimizeAway(sl);
                    }
                );
            }
            
            big_sp_deinit(&sl);
            free(ptrs);
            free(bigs);
        }
        
        if(bench_hetrohive)
        {
            // STABLE_POOL SETUP
            
            srand(69420);
            hetrohive sl; hh_init(&sl, 2);
            Big **ptrs = (Big**) malloc(sz * sizeof(ptrs[0]));
            Big *bigs = (Big*) malloc(sz * sizeof(bigs[0]));
            for (int i = 0; i < sz; i++)
                bigs[i] = (Big){.i = i};
            
            hh_register_type(&sl, 1, sizeof(Big));
            hh_put_all(&sl, bigs, sz, 1);
            
            int i = 0;
            
            for(auto it = hh_begin(&sl); !hh_iter_eq(it, hh_end(&sl)) ; it = hh_iter_next(it))
            {
                ptrs[i++] = (Big*) hh_iter_elm(it);
            }
            
            rng.seed(42);
            std::vector<Big *> to_pop;
            to_pop.reserve(sz / 2);
            std::sample(ptrs, ptrs + sz, std::back_inserter(to_pop), sz / 2, rng);
            
            for (Big *p : to_pop) {
                hh_del(&sl, p);
            }
            
            assert(sl.count == sz - sz/2);
            
            // STABLE_POOL END
            
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
                                Big elm = {.i = i};
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
                                int val = ((Big*)hh_iter_elm(it))->i;
                                it = hh_iter_del(&sl, it);
                                if(hh_iter_eq(it, hh_end(&sl)))
                                    iter_set = false;
                            }
                        }
                        
#if !defined(NDEBUG)
                        unsigned int sum = 0;
                        for(auto it = hh_begin(&sl), end = hh_end(&sl) ; !hh_iter_eq(it,end) ; hh_iter_go_next(&it))
                        {
                            sum += ((Big*)hh_iter_elm(it))->i;
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
                        
                        for(hh_iter it = hh_begin(&sl), end = hh_end(&sl) ; !hh_iter_eq(it,end) ; hh_iter_go_next(&it))
                        {
                            sum += ((Big*)hh_iter_elm(it))->i;
                        }
                        
                        ankerl::nanobench::doNotOptimizeAway(sum);
                        printf("hh_iter = %u\n", sum);
                    }
                );
                
                bench.unit("elms").batch(sz).complexityN(sz).minEpochIterations(iterations).run("hh_typed_iter",
                    [&]{
                        volatile unsigned int sum = 0;
                        
                        for(hh_typed_iter it = hh_typed_begin(&sl, 1), end = hh_typed_end(&sl, 1) ; !hh_typed_iter_eq(it,end) ; hh_typed_iter_go_next(&it))
                        {
                            sum += ((Big*)hh_typed_iter_elm(it))->i;
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
                        // big_sp slc = big_sp_clone(&sl);
                        
                        for(int i = 0 ; i < sz/2 ; i++)
                        {
                            Big elm = {.i=i};
                            hh_put(&sl, &elm, 1);
                        }
                        
#if !defined(NDEBUG)
                        unsigned int sum = 0;
                        for(auto it = hh_begin(&sl), end = hh_end(&sl) ; !hh_iter_eq(it,end) ; hh_iter_go_next(&it))
                        {
                            sum += ((Big*)hh_iter_elm(it))->i;
                        }
                        
                        printf("HH SUM AFTER PUT: %u\n", sum);
#endif
                        ankerl::nanobench::doNotOptimizeAway(sl);
                    }
                );
            }
            
            hh_deinit(&sl);
            free(ptrs);
            free(bigs);
        }
        
        if(bench_brace)
        {
            // BRACE SETUP
            
            srand(69420);
            bbrace sl; bbrace_init(&sl);
            Big **ptrs = (Big**) malloc(sz * sizeof(ptrs[0]));
            Big *bigs = (Big*) malloc(sz * sizeof(bigs[0]));
            for (int i = 0; i < sz; i++)
            {
                bigs[i] = (Big){.i = i};
                bbrace_put(&sl, bigs[i]);
            }
            
            int i = 0;
            
            for(auto it = bbrace_begin(&sl); !bbrace_iter_eq(it, bbrace_end(&sl)) ; it = bbrace_iter_next(it))
            {
                ptrs[i++] = bbrace_iter_elm(it);
            }
            
            rng.seed(42);
            std::vector<Big *> to_pop;
            to_pop.reserve(sz / 2);
            std::sample(ptrs, ptrs + sz, std::back_inserter(to_pop), sz / 2, rng);
            
            for (Big *p : to_pop) {
                bbrace_del(&sl, p);
            }
            
            assert(sl.count == sz - sz/2);
            
            
            
            // BRACE END
            if(bench_random)
            {
                rng2.seed(41);
                srand(69420);
                
                bench.unit("elms").batch(sz).complexityN(sz).minEpochIterations(iterations).run("bbrace_rnd",
                    [&]{
                        bbrace_iter_t it = {};
                        bool iter_set = false;
                        printf("COUNT = %zu\n", sl.count);
                        for(int i = 0, random = rand() % 100 ; i < sz ; i++, random = rand() % 100)
                        {
                            if(random <55 || sl.count == 0 || !iter_set)
                            {
                                bbrace_iter_t tmp = bbrace_put(&sl, (Big){.i=i});
                                
                                random = rand() % 100;
                                if(random < 5 || !iter_set)
                                {
                                    iter_set = true;
                                    it = tmp;
                                }
                            }
                            else
                            {
                                int val = bbrace_iter_elm(it)->i;
                                it = bbrace_iter_del(&sl, it);
                                if(bbrace_iter_eq(it, bbrace_end(&sl)))
                                    iter_set = false;
                            }
                        }
                        
#if !defined(NDEBUG)
                        unsigned int sum = 0;
                        for(auto it = bbrace_begin(&sl) ; !bbrace_iter_eq(it,bbrace_end(&sl)) ; it = bbrace_iter_next(it))
                        {
                            sum += bbrace_iter_elm(it)->i;
                        }
                        
                        printf("BRACE SUM AFTER RND: %u\n", sum);
#endif
                        ankerl::nanobench::doNotOptimizeAway(sl);
                    }
                );
            }
            
            
            if(bench_iter)
            {
                bench.unit("elms").batch(sz).complexityN(sz).minEpochIterations(iterations).run("bbrace_iter",
                    [&]{
                        volatile unsigned int sum = 0;
                        
                        for(auto it = bbrace_begin(&sl), end = bbrace_end(&sl) ; !bbrace_iter_eq(it, end) ; it = bbrace_iter_next(it))
                        {
                            sum += bbrace_iter_elm(it)->i;
                        }
                        
                        ankerl::nanobench::doNotOptimizeAway(sum);
                        printf("brace_iter = %u\n", sum);
                    }
                );
                
                bbrace_optimize(&sl);
                bench.unit("elms").batch(sz).complexityN(sz).minEpochIterations(iterations).run("bbrace_iter_opt",
                [&]{
                    volatile unsigned int sum = 0;
                    
                    for(auto it = bbrace_begin(&sl), end = bbrace_end(&sl); !bbrace_iter_eq(it, end) ; it = bbrace_iter_next(it))
                    {
                        sum += bbrace_iter_elm(it)->i;
                    }
                    
                    ankerl::nanobench::doNotOptimizeAway(sum);
                    printf("brace_iter = %u\n", sum);
                }
                );
            }
            
            if(bench_pop)
            {
                rng2.seed(41);
                bench.unit("elms").batch(sz).complexityN(sz).minEpochIterations(iterations).run("bbrace_del",
                    [&]{
                        bbrace_iter_del(&sl, bbrace_begin(&sl));
                        ankerl::nanobench::doNotOptimizeAway(sl);
                    }
                );
            }
            
            if(bench_put)
            {
                int d = 0;
                rng2.seed(41);
                bench.unit("elms").batch(sz).complexityN(sz).minEpochIterations(iterations).run("bbrace_put",
                    [&]{
                        printf("new iter %d\n", d++);
                        for(int i = 0 ; i < sz/2 ; i++)
                        {
                            bbrace_put(&sl, (Big){.i=i});
                        }
                        
#if !defined(NDEBUG)
                        unsigned int sum = 0;
                        for(auto it = bbrace_begin(&sl), end = bbrace_end(&sl) ; !bbrace_iter_eq(it, end) ; it = bbrace_iter_next(it))
                        {
                            sum += bbrace_iter_elm(it)->i;
                        }
                        
                        printf("BRACE SUM AFTER PUT: %u\n", sum);
#endif
                        ankerl::nanobench::doNotOptimizeAway(sl);
                    }
                );
            }
            
            bbrace_deinit(&sl);
            free(ptrs);
            free(bigs);
        }
        
        if(bench_small_hive)
        {
            // STABLE_POOL SETUP
            
            srand(69420);
            bbig_sp sl; bbig_sp_init(&sl);
            Big **ptrs = (Big**) malloc(sz * sizeof(ptrs[0]));
            Big *bigs = (Big*) malloc(sz * sizeof(bigs[0]));
            for (int i = 0; i < sz; i++)
                bigs[i] = (Big){.i = i};
            
            bbig_sp_put_all(&sl, bigs, sz);
            
            int i = 0;
            
            HIVE_FOREACH(&sl,
                       {
                           ptrs[i++] = HIVE_ITER_ELM;
                       });
            
            rng.seed(42);
            std::vector<Big*> to_pop;
            to_pop.reserve(sz / 2);
            std::sample(ptrs, ptrs + sz,
                        std::back_inserter(to_pop),
                        sz / 2,
                        rng);
            
            for (Big* p : to_pop) {
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
                        bbig_sp_iter_t it = {};
                        bool iter_set = false;
                        printf("COUNT = %zu\n", sl.count);
                        for(int i = 0, random = rand() % 100 ; i < sz ; i++, random = rand() % 100)
                        {
                            if(random <55 || sl.count == 0 || !iter_set)
                            {
                                bbig_sp_iter_t tmp = bbig_sp_put(&sl, (Big){.i=i});
                                
                                random = rand() % 100;
                                if(random < 5 || !iter_set)
                                {
                                    iter_set = true;
                                    it = tmp;
                                }
                            }
                            else
                            {
                                int val = it.elm->value.i;
                                it = bbig_sp_iter_del(&sl, it);
                                if(bbig_sp_iter_eq(it, bbig_sp_end(&sl)))
                                    iter_set = false;
                            }
                        }
                        
#if !defined(NDEBUG)
                        unsigned int sum = 0;
                        for(auto it = bbig_sp_begin(&sl), end = bbig_sp_end(&sl) ; !bbig_sp_iter_eq(it,end) ; bbig_sp_iter_go_next(&it))
                        {
                            sum += bbig_sp_iter_elm(it)->i;
                        }
                        
                        printf("HV SUM AFTER RND: %u\n", sum);
#endif
                        ankerl::nanobench::doNotOptimizeAway(sl);
                    }
                );
            }
            
            if(bench_iter)
            {
                bench.unit("elms").batch(sz).complexityN(sz).minEpochIterations(iterations).run("bhive_iter",
                [&]{
                    volatile unsigned int sum = 0;
                    
                    const bbig_sp_bucket_t *const end = sl.end_sentinel;
                    for(bbig_sp_iter_t it = bbig_sp_begin(&sl) ; it.bucket != end ; bbig_sp_iter_go_next(&it))
                    {
                        sum += bbig_sp_iter_elm(it)->i;
                    }
                    
                    ankerl::nanobench::doNotOptimizeAway(sum);
                    printf("hive_iter = %u\n", sum);
                }
                );
                bench.unit("elms").batch(sz).complexityN(sz).minEpochIterations(iterations).run("bhive_macro",
                [&]{
                    volatile unsigned int sum = 0;
                    
                    const bbig_sp_bucket_t *const end = sl.end_sentinel;
                    HIVE_FOREACH(&sl,
                               sum += HIVE_ITER_ELM->i;);
                    
                    ankerl::nanobench::doNotOptimizeAway(sum);
                    printf("hive_iter = %u\n", sum);
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
                            bbig_sp_put(&slc, (Big){.i=i});
                        }
                        
//                         unsigned int sum = 0;
//                         for(auto it = bbig_sp_begin(&slc) ; !bbig_sp_iter_is_end(it) ; bbig_sp_iter_go_next(&it))
//                         {
//                             sum += bbig_sp_iter_elm(it)->i;
//                         }
//                         
//                         printf("SP SUM AFTER PUT: %u\n", sum);
//                         ankerl::nanobench::doNotOptimizeAway(slc);
                        bbig_sp_deinit(&slc);
                    }
                );
            }
            
            if(bench_pop)
            {
                rng2.seed(41);
                bench.unit("elms").batch(sz).complexityN(sz).minEpochIterations(iterations).run("bhive_del",
                    [&]{
                        bbig_sp_iter_del(&sl, bbig_sp_begin(&sl));
                        
                        ankerl::nanobench::doNotOptimizeAway(sl);
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
            plf::colony<Big> i_colony;
            
            std::vector<decltype(i_colony.begin())> elms;
            elms.reserve(sz);
            for (int i = 0; i < sz; ++i)
            {
                elms.push_back(i_colony.insert((Big){.i = i}));
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
                                auto tmp = i_colony.insert((Big){.i=i});
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
                            sum += it->i;
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
                            sum += value.i;
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
                    plf::colony<Big> icol = plf::colony<Big>(i_colony);
                    bool remove = true;
                    for (plf::colony<Big>::iterator it = icol.begin() ; it != icol.end() ; )
                    {
                        if(remove)
                            it = icol.erase(it);
                        else
                            it++;
                        remove = !remove;
                    }
                    
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
                    // plf::colony<Big> icol = plf::colony<Big>(i_colony);
                    
                    for (int i = 0 ; i < sz/2 ; i++)
                    {
                        i_colony.insert((Big){.i=i});
                    }
#if !defined(NDEBUG)
                    unsigned int sum = 0;
                    for(auto it : i_colony)
                    {
                        sum += it.i;
                    }
                    
                    printf("PLF SUM AFTER INS %u\n", sum);
#endif
                    ankerl::nanobench::doNotOptimizeAway(i_colony);
                }
                );
            }
        }
        
        if(bench_stable_vec)
        {
            // STABLE_VECTOR SETUP
            
            srand(69420);
            boost::container::stable_vector<Big> stable_vec;
            
            std::vector<decltype(stable_vec.begin())> sv_elms;
            for (int i = 0; i < sz; ++i) {
                Big b;
                b.i = rand() - i;
                auto it = stable_vec.insert(stable_vec.end(), b);
                sv_elms.push_back(it);
            }
            
            rng.seed(42);
            std::vector<decltype(stable_vec.begin())> sv_to_erase;
            sv_to_erase.reserve(sz/2);
            std::sample(sv_elms.begin(), sv_elms.end(),
                        std::back_inserter(sv_to_erase),
                        sz/2,
                        rng);
            
            for (auto it : sv_to_erase)
            {
                stable_vec.erase(it);
            }
            
            assert(stable_vec.size() == sz/2);
            
            // STABLE_VECTOR END
            
            if(bench_iter)
            {
                bench.unit("elms").batch(sz).complexityN(sz).minEpochIterations(iterations).run("boost::stable_vector", 
                [&]{
                    volatile unsigned int sum = 0;
                    
                    for (const auto& value : stable_vec)
                    {
                        sum += value.i;
                    }
                    ankerl::nanobench::doNotOptimizeAway(sum);
                    printf("stable_vec_sum = %u\n", sum);
                }
                );
            }
            
            if(bench_random)
            {
                rng2.seed(41);
                srand(69420);
                bench.unit("elms").batch(sz).complexityN(sz).minEpochIterations(iterations).run("boost:stable_vec rnd",
                    [&]{
                        bool set_iter = false;
                        auto it = stable_vec.begin();
                        for(int i = 0, random = rand() % 100 ; i < sz ; i++, random = rand() % 100)
                        {
                            if(random <55 || stable_vec.size() == 0 || !set_iter)
                            {
                                stable_vec.push_back((Big){.i=i});
                                auto tmp = std::prev(stable_vec.end());
                                random = rand() % 100;
                                if(random < 5 || !set_iter)
                                {
                                    set_iter = true;
                                    it = tmp;
                                }
                            }
                            else
                            {
                                it = stable_vec.erase(it);
                                set_iter = false;
                            }
                        }
                        
#if !defined(NDEBUG)
                        unsigned int sum = 0;
                        for(auto it = stable_vec.begin() ; it != stable_vec.end() ; it++)
                        {
                            sum += it->i;
                        }
                        
                        printf("LS SUM AFTER RND: %u\n", sum);
#endif
                        ankerl::nanobench::doNotOptimizeAway(stable_vec);
                    }
                );
            }
        }
        
        if(bench_linked_list)
        {
            // LIST SETUP
            srand(69420);
            std::list<Big> ls;
            
            std::vector<decltype(ls.begin())> elms;
            elms.reserve(sz);
            for (int i = 0; i < sz; ++i)
            {
                ls.push_back((Big){.i = i});
                elms.push_back(std::prev(ls.end()));
            }
            
            rng.seed(42);
            std::vector<decltype(ls.begin())> to_erase;
            to_erase.reserve(sz/2);
            std::sample(elms.begin(), elms.end(),
                        std::back_inserter(to_erase),
                        sz/2,
                        rng);
            
            auto beg = to_erase.begin();
            auto end = to_erase.end();
            for ( ; beg != end ; beg++)
            {
                ls.erase(*beg);
            }
            
            // assert(i_colony.size() == sz/2);
            // PLF SETUP END
            
            if(bench_iter)
                bench.unit("elms").batch(sz).complexityN(sz).minEpochIterations(iterations).run("std::list", 
                    [&]{
                        volatile unsigned int sum = 0;
                        
                        for (const auto& value : ls)
                        {
                            sum += value.i;
                        }
                        ankerl::nanobench::doNotOptimizeAway(sum);
                        printf("lssum = %u\n", sum);
                        
                    }
                );
            
            if(bench_pop)
            {
                // std::cout << "SZ: " << sz << std::endl;
                rng2.seed(41);
                bench.unit("elms").batch(sz).complexityN(sz).minEpochIterations(iterations).run("std::list::erase", 
                [&]{
                    std::list<Big> ls2 = std::list<Big>(ls);
                    bool remove = true;
                    for (auto it = ls2.begin() ; it != ls2.end() ; )
                    {
                        if(remove)
                            it = ls2.erase(it);
                        else
                            it++;
                        remove = !remove;
                    }
                    
                    ankerl::nanobench::doNotOptimizeAway(ls2);
                }
                );
            }
            
            if(bench_put)
            {
                // std::cout << "SZ: " << sz << std::endl;
                rng2.seed(41);
                bench.unit("elms").batch(sz).complexityN(sz).minEpochIterations(iterations).run("std::list::insert", 
                [&]{
                    // plf::colony<Big> icol = plf::colony<Big>(i_colony);
                    
                    for (int i = 0 ; i < sz/2 ; i++)
                    {
                        ls.push_back((Big){.i=i});
                    }
#if !defined(NDEBUG)
                    unsigned int sum = 0;
                    for(auto it : ls)
                    {
                        sum += it.i;
                    }
                    
                    printf("LS SUM AFTER INS %u\n", sum);
#endif
                    ankerl::nanobench::doNotOptimizeAway(ls);
                }
                );
            }
            
            if(bench_random)
            {
                rng2.seed(41);
                srand(69420);
                bench.unit("elms").batch(sz).complexityN(sz).minEpochIterations(iterations).run("std::list rnd",
                    [&]{
                        bool set_iter = false;
                        auto it = ls.begin();
                        for(int i = 0, random = rand() % 100 ; i < sz ; i++, random = rand() % 100)
                        {
                            if(random <55 || ls.size() == 0 || !set_iter)
                            {
                                ls.push_back((Big){.i=i});
                                auto tmp = std::prev(ls.end());
                                random = rand() % 100;
                                if(random < 5 || !set_iter)
                                {
                                    set_iter = true;
                                    it = tmp;
                                }
                            }
                            else
                            {
                                it = ls.erase(it);
                                set_iter = false;
                            }
                        }
                        
#if !defined(NDEBUG)
                        unsigned int sum = 0;
                        for(auto it = ls.begin() ; it != ls.end() ; it++)
                        {
                            sum += it->i;
                        }
                        
                        printf("LS SUM AFTER RND: %u\n", sum);
#endif
                        ankerl::nanobench::doNotOptimizeAway(ls);
                    }
                );
            }

        }
        
        
        printf("Done from %d\n", sz);
    }
    std::ofstream htmlFile(html_file_name);
    std::ofstream jsonFile(json_file_name);
    
    bench.render(ankerl::nanobench::templates::htmlBoxplot(), htmlFile);
    bench.render(ankerl::nanobench::templates::json(), jsonFile);
    
    return 0;
}
