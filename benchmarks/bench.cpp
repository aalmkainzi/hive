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

#if !defined(NDEBUG)
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
#include "stable_pool.h"

#define HIVE_IMPL
#define HIVE_TYPE Big
#define HIVE_NAME bbig_sp
#if __has_include("../../sp_other/stable_pool.h")
#include "../../sp_other/stable_pool.h"
#else
#include "stable_pool.h"
#endif

// #define HIVE_TYPE Big
// #define HIVE_NAME hv
// #include "../../hive/hive.h"

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

int main()
{
    ankerl::nanobench::Bench bench;
    
    std::ofstream outFile(std::string("results/txt/hive_and_plf_colony_").append(compiler_name).append(std::string_view(".txt")));
    bench.output(&outFile);
    
    int begin = 25'000;
    int end   = 125'000;
    int interval = 13'543;
    
    std::string html_file_name = std::string("results/html/hive_and_plf_colony_").append(compiler_name).append(".html");
    std::string json_file_name = std::string("results/json/hive_and_plf_colony_").append(compiler_name).append(".json");
    
    constexpr bool bench_hive = true;
    constexpr bool bench_small_hive = false;
    constexpr bool bench_plf_colony  = true;
    constexpr bool bench_slot_map    = false;
    constexpr bool bench_stable_vec  = false;
    constexpr bool bench_linked_list = false;
    
    constexpr bool bench_iter = false;
    constexpr bool bench_put = false;
    constexpr bool bench_pop = false;
    constexpr bool bench_random = true;
    
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
                bigs[i] = (Big){.i = rand() - i};
            
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
            
            // assert(sl.count == sz/2);
            
            // STABLE_POOL END
            
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
                        
                        for(big_sp_iter_t it = big_sp_begin(&sl) ; !big_sp_iter_is_end(it) ; big_sp_iter_go_next(&it))
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
                        big_sp slc = big_sp_clone(&sl);
                        
                        bool remove = true;
                        for(big_sp_iter_t it = big_sp_begin(&slc) ; !big_sp_iter_is_end(it) ; )
                        {
                            if(remove)
                                it = big_sp_iter_del(it);
                            else
                                big_sp_iter_go_next(&it);
                            remove = !remove;
                        }
                        
                        ankerl::nanobench::doNotOptimizeAway(slc);
                        big_sp_deinit(&slc);
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
                        for(auto it = big_sp_begin(&sl) ; !big_sp_iter_is_end(it) ; big_sp_iter_go_next(&it))
                        {
                            sum += big_sp_iter_elm(it)->i;
                        }
                        
                        printf("HV SUM AFTER PUT: %u\n", sum);
#endif
                        ankerl::nanobench::doNotOptimizeAway(sl);
                    }
                );
            }
            
            if(bench_random)
            {
                rng2.seed(41);
                srand(69420);
                bench.unit("elms").batch(sz).complexityN(sz).minEpochIterations(iterations).run("hive_rnd",
                    [&]{
                        big_sp_iter_t it = {};
                        bool iter_set = false;
                        for(int i = 0, random = rand() % 100 ; i < sz ; i++, random = rand() % 100)
                        {
                            if(random < 75 || sl.count == 0 || !iter_set)
                            {
                                big_sp_iter_t tmp = big_sp_put(&sl, (Big){.i=i});
                                random = rand() % 100;
                                if(random < 5 || !iter_set)
                                {
                                    iter_set = true;
                                    it = tmp;
                                }
                            }
                            else
                            {
                                it = big_sp_iter_del(it);
                                iter_set = false;
                            }
                        }
                        
#if !defined(NDEBUG)
                        unsigned int sum = 0;
                        for(auto it = big_sp_begin(&sl) ; !big_sp_iter_is_end(it) ; big_sp_iter_go_next(&it))
                        {
                            sum += big_sp_iter_elm(it)->i;
                        }
                        
                        printf("HV SUM AFTER RND: %u\n", sum);
#endif
                        ankerl::nanobench::doNotOptimizeAway(sl);
                    }
                );
            }
            
            big_sp_deinit(&sl);
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
                bigs[i] = (Big){.i = rand() - i};
            
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
                        bbig_sp slc = bbig_sp_clone(&sl);
                        
                        bool remove = true;
                        for(bbig_sp_iter_t it = bbig_sp_begin(&slc) ; !bbig_sp_iter_is_end(it) ; )
                        {
                            if(remove)
                                it = bbig_sp_iter_del(it);
                            else
                                bbig_sp_iter_go_next(&it);
                            remove = !remove;
                        }
                        
                        ankerl::nanobench::doNotOptimizeAway(slc);
                        bbig_sp_deinit(&slc);
                    }
                );
            }
            
            if(bench_random)
            {
                rng2.seed(41);
                srand(69420);
                bench.unit("elms").batch(sz).complexityN(sz).minEpochIterations(iterations).run("bhive_rnd",
                    [&]{
                        bbig_sp_iter_t it = {};
                        bool iter_set = false;
                        for(int i = 0, random = rand() % 100 ; i < sz ; i++, random = rand() % 100)
                        {
                            if(random < 75 || sl.count == 0 || !iter_set)
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
                                it = bbig_sp_iter_del(it);
                                iter_set = false;
                            }
                        }
                        
#if !defined(NDEBUG)
                        unsigned int sum = 0;
                        for(auto it = bbig_sp_begin(&sl) ; !bbig_sp_iter_is_end(it) ; bbig_sp_iter_go_next(&it))
                        {
                            sum += bbig_sp_iter_elm(it)->i;
                        }
                        
                        printf("BHV SUM AFTER RND: %u\n", sum);
#endif
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
                elms.push_back(i_colony.insert((Big){.i = rand() - i}));
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
            
            // assert(i_colony.size() == sz/2);
            // PLF SETUP END
            
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
                            if(random < 75 || i_colony.size() == 0 || !set_iter)
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
        }
        
        if(bench_slot_map)
        {
            // SLOT MAP SETUP
            
            srand(69420);
            dod::slot_map<Big> slot_map;
            
            std::vector<dod::slot_map_key64<Big>> slot_elms;
            for (int i = 0; i < sz; ++i) {
                Big b;
                b.i = rand() - i;
                dod::slot_map_key64<Big> lsit = slot_map.emplace(b);
                slot_elms.push_back(lsit);
            }
            
            rng.seed(42);
            std::vector<dod::slot_map_key64<Big>> sm_to_erase;
            sm_to_erase.reserve(sz/2);
            std::sample(slot_elms.begin(), slot_elms.end(),
                        std::back_inserter(sm_to_erase),
                        sz/2,
                        rng);
            
            for (auto it : sm_to_erase)
            {
                slot_map.pop(it);
            }
            
            assert(slot_map.size() == sz/2);
            // SLOT MAP END
            
            bench.unit("elms").batch(sz).complexityN(sz).minEpochIterations(iterations).run("dod::slot_map", 
                [&]{
                    volatile unsigned int sum = 0;
                    
                    for (const auto& value : slot_map)
                    {
                        sum += value.i;
                    }
                    ankerl::nanobench::doNotOptimizeAway(sum);
                    printf("slotsum = %u\n", sum);
                    
                }
            );
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
        
        if(bench_linked_list)
        {
            // LINKED LIST SETUP
            
            std::forward_list<Big> flist;
            auto last = flist.before_begin();               // points to before the first
            std::vector<decltype(flist.begin())> elems;     // will store iterators to each element
            
            srand(69420);
            for (int i = 0; i < sz; ++i) {
                Big b;
                b.i = rand() - i;
                // insert AFTER last, then update last to the newly inserted element
                last = flist.insert_after(last, b);
                elems.push_back(last);
            }
            
            // --- 2) pick half of them at random to erase ---
            std::mt19937_64 rng{42};
            std::vector<decltype(flist.begin())> to_erase;
            to_erase.reserve(sz/2);
            std::sample(elems.begin(), elems.end(),
                        std::back_inserter(to_erase),
                        sz/2,
                        rng);
            
            // --- 3) erase each one by first finding its "previous" node ---
            for (auto it : to_erase) {
                auto prev = flist.before_begin();
                // walk until prev->next == it
                while (std::next(prev) != it) {
                    ++prev;
                }
                // erase the node after prev
                flist.erase_after(prev);
            }
            
            assert(std::distance(flist.begin(), flist.end()) == sz/2);
            // LINKED LIST END
            bench.unit("elms").batch(sz).complexityN(sz).minEpochIterations(iterations).run("std::forward_list", 
                [&]{
                    volatile unsigned int sum = 0;
                    
                    for (const auto& value : flist)
                    {
                        sum += value.i;
                    }
                    ankerl::nanobench::doNotOptimizeAway(sum);
                    printf("forward_list_sum = %u\n", sum);
                }
            );
        }
        
        
        printf("Done from %d\n", sz);
    }
    std::ofstream htmlFile(html_file_name);
    std::ofstream jsonFile(json_file_name);
    
    bench.render(ankerl::nanobench::templates::htmlBoxplot(), htmlFile);
    bench.render(ankerl::nanobench::templates::json(), jsonFile);
    
    return 0;
}