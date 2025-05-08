#include <iostream>
#include <list>
#include <iterator>
#include <random>
#include <boost/container/stable_vector.hpp>
#include "plf_colony.h"
#include "plf_list.h"
#include "slot_map.h"

#define printf(...) // printf(__VA_ARGS__)

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
#define SP_IMPL
#define SP_TYPE Big
#define SP_NAME big_sp

#include "stable_pool.h"
}

// static void BM_stable_vector_iteration(benchmark::State& state)
// {
//     srand(69420);
//     boost::container::stable_vector<Big> ls;
//     const int N = state.range(0);
//
//     std::vector<decltype(ls.begin())> elms;
//     for (int i = 0; i < N; ++i) {
//         Big b;
//         b.i = rand() - i;
//         ls.push_back(b);
//         auto lsit = ls.begin();
//         std::advance(lsit, i);
//         elms.push_back(lsit);
//     }
//
//     std::mt19937 rng(42);
//     std::vector<decltype(ls.begin())> to_erase;
//     to_erase.reserve(N/2);
//     std::sample(elms.begin(), elms.end(),
//                 std::back_inserter(to_erase),
//                 N/2,
//                 rng);
//
//     for (auto it : to_erase)
//     {
//         ls.erase(it);
//     }
//
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
//
//         printf("sv sum = %u\n", sum);
//     }
// }
//
extern "C" void accumSum(Big *elm, void *arg)
{
    *(unsigned int*)arg += elm->i;
}
//
// static void BM_set_iteration(benchmark::State& state)
// {
//     srand(69420);
//     big_set set;
//     big_set_init(&set);
//     const int N = state.range(0);
//
//     std::vector<Big> elms;
//     for (int i = 0; i < N; ++i)
//     {
//         Big b;
//         b.i = rand() - i;
//         big_set_itr lsit = big_set_insert(&set, b);
//         elms.push_back(b);
//     }
//
//     std::mt19937 rng(42);
//     std::vector<Big> to_erase;
//     to_erase.reserve(N/2);
//     std::sample(elms.begin(), elms.end(),
//                 std::back_inserter(to_erase),
//                 N/2,
//                 rng);
//
//     for (auto it : to_erase)
//     {
//         Big bsi = it;
//         big_set_erase(&set, bsi);
//     }
//
//     volatile unsigned int sum = 0;
//     for (auto _ : state)
//     {
//         for (big_set_itr itr = big_set_first( &set );
//              !big_set_is_end( itr );
//              itr = big_set_next( itr ))
//         {
//             sum += itr.data->key.i;
//         }
//
//         benchmark::ClobberMemory();
//         benchmark::DoNotOptimize(sum);
//
//         printf("set sum = %u\n", sum);
//     }
//
//     big_set_cleanup(&set);
// }
//
// static void BM_List_Iteration(benchmark::State& state)
// {
//     srand(69420);
//     std::list<Big> ls;
//     const int N = state.range(0);
//
//     std::vector<decltype(ls.begin())> elms;
//     for (int i = 0; i < N; ++i) {
//         Big b;
//         b.i = rand() - i;
//         ls.push_back(b);
//         auto lsit = ls.begin();
//         std::advance(lsit, i);
//         elms.push_back(lsit);
//     }
//
//     std::mt19937 rng(42);
//     std::vector<decltype(ls.begin())> to_erase;
//     to_erase.reserve(N/2);
//     std::sample(elms.begin(), elms.end(),
//                 std::back_inserter(to_erase),
//                 N/2,
//                 rng);
//
//     for (auto it : to_erase)
//     {
//         ls.erase(it);
//     }
//
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
//
//         printf("list sum = %u\n", sum);
//     }
// }
//
// static void BM_PLFList_Iteration(benchmark::State& state)
// {
//     srand(69420);
//     plf::list<Big> ls;
//     const int N = state.range(0);
//
//     std::vector<decltype(ls.begin())> elms;
//     for (int i = 0; i < N; ++i) {
//         Big b;
//         b.i = rand() - i;
//         ls.push_back(b);
//         auto lsit = ls.begin();
//         std::advance(lsit, i);
//         elms.push_back(lsit);
//     }
//
//     std::mt19937 rng(42);
//     std::vector<decltype(ls.begin())> to_erase;
//     to_erase.reserve(N/2);
//     std::sample(elms.begin(), elms.end(),
//                 std::back_inserter(to_erase),
//                 N/2,
//                 rng);
//
//     for (auto it : to_erase)
//     {
//         ls.erase(it);
//     }
//
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
//
//         printf("list sum = %u\n", sum);
//     }
// }
//
//
// static void BM_stable_pool(benchmark::State& state) {
//     // Perform setup here
//     srand(69420);
//     big_sp sl; big_sp_init(&sl);
//     const int M = state.range(0);
//     Big **ptrs = (Big**) malloc(M * sizeof(ptrs[0]));
//     for (int i = 0; i < M; i++)
//         ptrs[i] = big_sp_put(&sl, (Big){.i = rand() - i});
//
//     std::mt19937 rng(42);
//     std::vector<Big*> to_pop;
//     to_pop.reserve(M / 2);
//     std::sample(ptrs, ptrs + M,
//                 std::back_inserter(to_pop),
//                 M / 2,
//                 rng);
//
//     for (Big* p : to_pop) {
//         big_sp_pop(&sl, p);
//     }
//
//     volatile unsigned int sum = 0;
//     big_sp *ssl = &sl;
//     for (auto _ : state) {
//
//         SP_FOREACH(ssl, sum += SP_IT->i; );
//
//         benchmark::ClobberMemory();
//         benchmark::DoNotOptimize(sum);
//
//         printf("stepsum = %u\n", sum);
//     }
//
//     free(ptrs);
//     big_sp_deinit(&sl);
// }
//
// static void BM_stable_pool_iter(benchmark::State& state) {
//     // Perform setup here
//     srand(69420);
//     big_sp sl; big_sp_init(&sl);
//     const int M = state.range(0);
//     Big **ptrs = (Big**) malloc(M * sizeof(ptrs[0]));
//     for (int i = 0; i < M; i++)
//         ptrs[i] = big_sp_put(&sl, (Big){.i = rand() - i});
//
//     std::mt19937 rng(42);
//     std::vector<Big*> to_pop;
//     to_pop.reserve(M / 2);
//     std::sample(ptrs, ptrs + M,
//                 std::back_inserter(to_pop),
//                 M / 2,
//                 rng);
//
//     for (Big* p : to_pop) {
//         big_sp_pop(&sl, p);
//     }
//
//
//     volatile unsigned int sum = 0;
//     big_sp *ssl = &sl;
//     for (auto _ : state) {
//
//         for(big_sp_iter_t it  = big_sp_begin(&sl),
//                           end = big_sp_end(&sl)  ;
//             !big_sp_iter_eq(it, end)             ;
//             it = big_sp_iter_next(it) )
//         {
//             sum += big_sp_iter_elm(it)->i;
//         }
//
//         benchmark::ClobberMemory();
//         benchmark::DoNotOptimize(sum);
//
//          printf("step_it_sum = %u\n", sum);
//     }
//
//     free(ptrs);
//     big_sp_deinit(&sl);
// }
//
void add_sum(Big *big, void *arg)
{
    *(unsigned int*)arg += big->i;
}
//
// static void BM_stable_pool_func(benchmark::State& state) {
//     // Perform setup here
//     srand(69420);
//     big_sp sl; big_sp_init(&sl);
//     const int M = state.range(0);
//     Big **ptrs = (Big**) malloc(M * sizeof(ptrs[0]));
//     for (int i = 0; i < M; i++)
//         ptrs[i] = big_sp_put(&sl, (Big){.i = rand() - i});
//
//     std::mt19937 rng(42);
//     std::vector<Big*> to_pop;
//     to_pop.reserve(M / 2);
//     std::sample(ptrs, ptrs + M,
//                 std::back_inserter(to_pop),
//                 M / 2,
//                 rng);
//     for (Big* p : to_pop) {
//         big_sp_pop(&sl, p);
//     }
//     volatile unsigned int sum = 0;
//     big_sp *ssl = &sl;
//     // printf("sl size = %zu\n", sl.count);
//     for (auto _ : state) {
//
//         big_sp_foreach(ssl, add_sum, (void*)&sum);
//
//         benchmark::ClobberMemory();
//         benchmark::DoNotOptimize(sum);
//
//          printf("step_func_sum = %u\n", sum);
//     }
//
//     free(ptrs);
//     big_sp_deinit(&sl);
// }
//
// static void BM_PLFColony_Iteration(benchmark::State& state)
// {
//     srand(69420);
//     plf::colony<Big> i_colony;
//     const int N = state.range(0);
//
//     std::vector<decltype(i_colony.begin())> elms;
//     elms.reserve(N);
//     for (int i = 0; i < N; ++i)
//     {
//         elms.push_back(i_colony.insert((Big){.i = rand() - i}));
//     }
//
//     std::mt19937 rng(42);
//     std::vector<decltype(i_colony.begin())> to_erase;
//     to_erase.reserve(N/2);
//     std::sample(elms.begin(), elms.end(),
//                 std::back_inserter(to_erase),
//                 N/2,
//                 rng);
//
//     for (auto it : to_erase)
//     {
//         i_colony.erase(it);
//     }
//
//     volatile unsigned int sum = 0;
//     for (auto _ : state)
//     {
//         for (const auto& value : i_colony)
//         {
//             sum += value.i;
//         }
//         benchmark::ClobberMemory();
//         benchmark::DoNotOptimize(sum);
//         printf("plfsum = %u\n", sum);
//     }
// }

#define ANKERL_NANOBENCH_IMPLEMENT
#include "nanobench.h"
#include <string>

// int main2()
// {
//     ankerl::nanobench::Bench bench;
//     
//     int sizes[] = {
//         // 10,
//         // 100,
//         //1000,
//         10000,
//         100000,
//     };
//     
//     int iterations = 1000;
//     
//     for(int& sz : sizes)
//     {
//         // PLF SETUP
//         srand(69420);
//         plf::colony<Big> i_colony;
//         const int N = sz;
//         
//         std::vector<decltype(i_colony.begin())> elms;
//         elms.reserve(N);
//         for (int i = 0; i < N; ++i)
//         {
//             elms.push_back(i_colony.insert((Big){.i = rand() - i}));
//         }
//         
//         std::mt19937 rng(42);
//         std::vector<decltype(i_colony.begin())> to_erase;
//         to_erase.reserve(N/2);
//         std::sample(elms.begin(), elms.end(),
//                     std::back_inserter(to_erase),
//                     N/2,
//                     rng);
//         
//         auto beg = to_erase.begin();
//         auto end = to_erase.end();
//         for ( ; beg != end ; beg++)
//         {
//             i_colony.erase(*beg);
//         }
//         // PLF SETUP END
//         
//         // STABLE_POOL SETUP
//         
//         srand(69420);
//         big_sp sl; big_sp_init(&sl);
//         const int M = sz;
//         Big **ptrs = (Big**) malloc(M * sizeof(ptrs[0]));
//         for (int i = 0; i < M; i++)
//             ptrs[i] = big_sp_put(&sl, (Big){.i = rand() - i});
//         
//         std::mt19937 rng2(42);
//         std::vector<Big*> to_pop;
//         to_pop.reserve(M / 2);
//         std::sample(ptrs, ptrs + M,
//                     std::back_inserter(to_pop),
//                     M / 2,
//                     rng2);
//         
//         for (Big* p : to_pop) {
//             big_sp_pop(&sl, p);
//         }
//         
//         // STABLE_POOL END
//         
//         bench.complexityN(sz).name("plf:" + std::to_string(sz)).minEpochIterations(iterations).run(
//             [&]{
//                 volatile unsigned int sum = 0;
//                 {
//                     for (const auto& value : i_colony)
//                     {
//                         sum += value.i;
//                     }
//                     ankerl::nanobench::doNotOptimizeAway(sum);
//                     printf("plfsum = %u\n", sum);
//                 }
//             }
//         );
//         
//         bench.complexityN(sz).name("stable_pool:" + std::to_string(sz)).minEpochIterations(iterations).run(
//             [&]{
//                 volatile unsigned int sum = 0;
//                 
//                 SP_FOREACH(&sl, sum += SP_IT->i; );
//                 
//                 ankerl::nanobench::doNotOptimizeAway(sum);
//                 printf("stable_pool = %u\n", sum);
//             }
//         );
//         
//         bench.complexityN(sz).name("stable_pool_func:" + std::to_string(sz)).minEpochIterations(iterations).run(
//             [&]{
//                 volatile unsigned int sum = 0;
//                 
//                 big_sp_foreach(&sl, add_sum, (void*) &sum);
//                 
//                 ankerl::nanobench::doNotOptimizeAway(sum);
//                 printf("stable_pool_func = %u\n", sum);
//             }
//         );
//         
//         bench.complexityN(sz).name("stable_pool_iter:" + std::to_string(sz)).minEpochIterations(iterations).run(
//             [&]{
//                 volatile unsigned int sum = 0;
//                 
//                 for(big_sp_iter_t it = big_sp_begin(&sl), end = big_sp_end(&sl) ; !big_sp_iter_eq(it,end) ; it = big_sp_iter_next(it))
//                 {
//                     sum += it.elm_entry->value.i;
//                 }
//                 
//                 ankerl::nanobench::doNotOptimizeAway(sum);
//                 printf("stable_pool_iter = %u\n", sum);
//             }
//         );
// 
//         big_sp_deinit(&sl);
//     }
//     std::ofstream outFile("out.html");
//     bench.render(ankerl::nanobench::templates::htmlBoxplot(), outFile);
//     
//     return 0;
// }

__attribute__((noinline))
void iter(big_sp *sp)
{
    volatile unsigned int sum = 0;
    
    for(int i = 0 ; i < 500 ; i++)
    {
        sum = 0;
        
        big_sp_foreach(sp, accumSum, (void*)&sum);
        // for(big_sp_iter_t it = big_sp_begin(&sl), end = big_sp_end(&sl) ; !big_sp_iter_eq(it, end) ; it = big_sp_iter_next(it))
        // {
        //     sum += big_sp_iter_elm(it)->i;
        // }
        printf("%u ", sum);
    }
}

int main()
{
    ankerl::nanobench::Bench bench;
    
    int sizes[] = {
        //1 << 10,
        //1 << 11,
        1 << 12,
        // 1 << 13,
        // 1 << 14,
        //1 << 15,
        // 1 << 16,
        1 << 17,
        1 << 18,
        1 << 19,
        //1 << 20,
    };
    
    int iterations = 10;
    
    for(int& sz : sizes)
    {
        printf("Starting %d\n", sz);
        
        // PLF SETUP
        srand(69420);
        plf::colony<Big> i_colony;
        const int N = sz;
        
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
        
        auto beg = to_erase.begin();
        auto end = to_erase.end();
        for ( ; beg != end ; beg++)
        {
            i_colony.erase(*beg);
        }
        
        assert(i_colony.size() == sz/2);
        // PLF SETUP END
        
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
        
        // SLOT MAP SETUP
        
        srand(69420);
        dod::slot_map<Big> slot_map;
        
        std::vector<dod::slot_map_key64<Big>> slot_elms;
        for (int i = 0; i < N; ++i) {
            Big b;
            b.i = rand() - i;
            dod::slot_map_key64<Big> lsit = slot_map.emplace(b);
            slot_elms.push_back(lsit);
        }
        
        rng.seed(42);
        std::vector<dod::slot_map_key64<Big>> sm_to_erase;
        sm_to_erase.reserve(N/2);
        std::sample(slot_elms.begin(), slot_elms.end(),
                    std::back_inserter(sm_to_erase),
                    N/2,
                    rng);
        
        for (auto it : sm_to_erase)
        {
            slot_map.pop(it);
        }
        
        assert(slot_map.size() == sz/2);
        // SLOT MAP END
        
        // STABLE_VECTOR SETUP
        
        srand(69420);
        boost::container::stable_vector<Big> stable_vec;
    
        std::vector<decltype(stable_vec.begin())> sv_elms;
        for (int i = 0; i < N; ++i) {
            Big b;
            b.i = rand() - i;
            auto it = stable_vec.insert(stable_vec.end(), b);
            sv_elms.push_back(it);
        }
    
        rng.seed(42);
        std::vector<decltype(stable_vec.begin())> sv_to_erase;
        sv_to_erase.reserve(N/2);
        std::sample(sv_elms.begin(), sv_elms.end(),
                    std::back_inserter(sv_to_erase),
                    N/2,
                    rng);
    
        for (auto it : sv_to_erase)
        {
            stable_vec.erase(it);
        }
        
        assert(stable_vec.size() == sz/2);
        
        // STABLE_VECTOR END
        
        // LINKED LIST SETUP
        
        srand(69420);
        std::list<Big> linked_list;
        
        std::vector<decltype(linked_list.begin())> ls_elms;
        for (int i = 0; i < N; ++i) {
            Big b;
            b.i = rand() - i;
            auto it = linked_list.insert(linked_list.end(), b);
            ls_elms.push_back(it);
        }
        
        rng.seed(42);
        std::vector<decltype(linked_list.begin())> ls_to_erase;
        ls_to_erase.reserve(N/2);
        std::sample(ls_elms.begin(), ls_elms.end(),
                    std::back_inserter(ls_to_erase),
                    N/2,
                    rng);
        
        for (auto it : ls_to_erase)
        {
            linked_list.erase(it);
        }
        
        assert(linked_list.size() == sz/2);
        // LINKED LIST END
        
        bench.complexityN(sz).name("plf::colony").minEpochIterations(iterations).run(
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
        
        std::cout.flush();
        
        bench.complexityN(sz).name("stable_pool").minEpochIterations(iterations).run(
            [&]{
                volatile unsigned int sum = 0;
                
                SP_FOREACH(&sl, sum += SP_IT->i; );
                
                ankerl::nanobench::doNotOptimizeAway(sum);
                printf("stable_pool = %u\n", sum);
            }
        );
        std::cout.flush();
        
        bench.complexityN(sz).name("stable_pool_func").minEpochIterations(iterations).run(
            [&]{
                volatile unsigned int sum = 0;
                
                big_sp_foreach(&sl, add_sum, (void*) &sum);
                
                ankerl::nanobench::doNotOptimizeAway(sum);
                printf("stable_pool_func = %u\n", sum);
            }
        );
        std::cout.flush();
        
        bench.complexityN(sz).name("stable_pool_iter").minEpochIterations(iterations).run(
            [&]{
                volatile unsigned int sum = 0;
                
                for(big_sp_iter_t it = big_sp_begin(&sl), end = big_sp_end(&sl) ; !big_sp_iter_eq(it,end) ; it = big_sp_iter_next(it))
                {
                    sum += big_sp_iter_elm(it)->i;
                }
                
                ankerl::nanobench::doNotOptimizeAway(sum);
                printf("stable_pool_iter = %u\n", sum);
            }
        );
        std::cout.flush();
        
        bench.complexityN(sz).name("dod::slot_map").minEpochIterations(iterations).run(
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
        std::cout.flush();
        
        bench.complexityN(sz).name("boost::stable_vector").minEpochIterations(iterations).run(
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
        std::cout.flush();
        
        bench.complexityN(sz).name("std::list").minEpochIterations(iterations).run(
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
        std::cout.flush();
        
        printf("Done from %d\n", sz);
        big_sp_deinit(&sl);
        free(ptrs);
        free(bigs);
    }
    std::ofstream outFile("out.html");
    bench.render(ankerl::nanobench::templates::htmlBoxplot(), outFile);
    return 0;
}
