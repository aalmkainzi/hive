#include <iostream>
#include <list>
#include <forward_list>
#include <iterator>
#include <random>
#include <boost/container/stable_vector.hpp>
#include "plf_colony.h"
#include "plf_list.h"
#include "slot_map.h"

#define printf(...) // printf(__VA_ARGS__)

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

#define SP_IMPL
#define SP_TYPE Big
#define SP_NAME big_sp
#include "stable_pool.h"

#define SP_IMPL
#define SP_TYPE Big
#define SP_NAME bbig_sp
#if __has_include("../../sp_other/stable_pool.h")
#include "../../sp_other/stable_pool.h"
#else
#include "stable_pool.h"
#endif

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
    
    std::ofstream outFile(std::string("results/txt/stable_pool_and_plf_colony_").append(compiler_name).append(std::string_view(".txt")));
    bench.output(&outFile);
    
    int begin = 100'000;
    int end = 800'000;
    int interval = 5000;
    
    std::string html_file_name = std::string("results/html/stable_pool_and_plf_colony_").append(compiler_name).append(".html");
    std::string json_file_name = std::string("results/json/stable_pool_and_plf_colony_").append(compiler_name).append(".json");
    
    constexpr bool bench_stable_pool = true;
    constexpr bool bench_small_stable_pool = false;
    constexpr bool bench_plf_colony  = true;
    constexpr bool bench_slot_map    = false;
    constexpr bool bench_stable_vec  = false;
    constexpr bool bench_linked_list = false;
    
    int iterations = 50;
    
    for(int sz = begin ; sz <= end ; sz += interval)
    {
        printf("Starting %d\n", sz);
        std::mt19937 rng(42);
        
        if(bench_stable_pool)
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
            
            SP_FOREACH(&sl,
            {
                ptrs[i++] = SP_IT;
            });
            
            rng.seed(42);
            std::vector<Big*> to_pop;
            to_pop.reserve(sz / 2);
            std::sample(ptrs, ptrs + sz,
                        std::back_inserter(to_pop),
                        sz / 2,
                        rng);
            
            for (Big* p : to_pop) {
                big_sp_pop(&sl, p);
            }
            
            assert(sl.count == sz/2);
            
            // STABLE_POOL END
            
            bench.unit("elms").batch(sz).complexityN(sz).minEpochIterations(iterations).run("stable_pool_macro", 
                [&]{
                    volatile unsigned int sum = 0;
                    
                    SP_FOREACH(&sl, sum += SP_IT->i; );
                    
                    ankerl::nanobench::doNotOptimizeAway(sum);
                    printf("stable_pool = %u\n", sum);
                }
            );
            std::cout.flush();
//         
//             bench.unit("elms").batch(sz).complexityN(sz).minEpochIterations(iterations).run("stable_pool_func", 
//                 [&]{
//                     volatile unsigned int sum = 0;
//                     
//                     big_sp_foreach(&sl, add_sum, (void*) &sum);
//                     
//                     ankerl::nanobench::doNotOptimizeAway(sum);
//                     printf("stable_pool_func = %u\n", sum);
//                 }
//             );
//             std::cout.flush();
             
            bench.unit("elms").batch(sz).complexityN(sz).minEpochIterations(iterations).run("stable_pool_iter",
                [&]{
                    volatile unsigned int sum = 0;
                    
                    const big_sp_bucket_t *const end = sl.end_sentinel;
                    for(big_sp_iter_t it = big_sp_begin(&sl) ; it.bucket != end ; big_sp_iter_go_next(&it))
                    {
                        sum += big_sp_iter_elm(it)->i;
                    }
                    
                    ankerl::nanobench::doNotOptimizeAway(sum);
                    printf("stable_pool_iter = %u\n", sum);
                }
            );
            
            big_sp_deinit(&sl);
            free(ptrs);
            free(bigs);
        }
        
        if(bench_small_stable_pool)
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
            
            SPo_FOREACH(&sl,
                       {
                           ptrs[i++] = SPo_IT;
                       });
            
            rng.seed(42);
            std::vector<Big*> to_pop;
            to_pop.reserve(sz / 2);
            std::sample(ptrs, ptrs + sz,
                        std::back_inserter(to_pop),
                        sz / 2,
                        rng);
            
            for (Big* p : to_pop) {
                bbig_sp_pop(&sl, p);
            }
            
            assert(sl.count == sz/2);
            
            // STABLE_POOL END
            
            bench.unit("elms").batch(sz).complexityN(sz).minEpochIterations(iterations).run("bstable_pool_iter",
                [&]{
                    volatile unsigned int sum = 0;
                    
                    const bbig_sp_bucket_t *const end = sl.end_sentinel;
                    for(bbig_sp_iter_t it = bbig_sp_begin(&sl) ; it.bucket != end ; bbig_sp_iter_go_next(&it))
                    {
                        sum += bbig_sp_iter_elm(it)->i;
                    }
                    
                    ankerl::nanobench::doNotOptimizeAway(sum);
                    printf("stable_pool_iter = %u\n", sum);
                }
            );
            
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
            
            assert(i_colony.size() == sz/2);
            // PLF SETUP END
            
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
            
            std::srand(69420);
            for (int i = 0; i < sz; ++i) {
                Big b;
                b.i = std::rand() - i;
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
// TODO add rust's colony
