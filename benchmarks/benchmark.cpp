#define BPPTREE_TEST_COUNT_ALLOCATIONS

#include <iostream>
#include <chrono>
#include <map>
#include "test_common.hpp"
#include "immer/flex_vector.hpp"

using namespace std;

template <typename N, auto n>
inline void run_benchmark(std::integral_constant<N, n>){
    Vector<int32_t> const& rand_ints = RandInts<int32_t, n>::ints;
    if constexpr (true) {
        using TreeType = BppTreeMap<int, int>::Transient;
        TreeType tree{};
        cout << "BppTreeMap<int, int>::Transient : " << n << endl;
        cout << "=============================================================" << endl;
        auto startTime = std::chrono::steady_clock::now();
        for (int i = 0; i < n; i++) {
            tree[rand_ints[i]] = i;
        }
        auto endTime = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsed = endTime - startTime;
        cout << elapsed.count() << 's' << endl;

        int64_t sum = 0;
        startTime = std::chrono::steady_clock::now();
        for (int i = 0; i < n; i++) {
            sum += tree[rand_ints[i]];
        }
        endTime = std::chrono::steady_clock::now();
        elapsed = endTime - startTime;
        cout << elapsed.count() << 's' << endl;
        cout << "allocations: " << allocations << " deallocations : " << deallocations << endl;
        cout << "increments: " << increments << " decrements : " << decrements << endl;
        cout << sum << endl << endl;
        reset_counters();
    }
    if constexpr (true) {
        using TreeType = BppTreeMap<int, int>::internal_node_bytes<256>::leaf_node_bytes<1024>::Persistent;
        TreeType tree{};
        cout << "BppTreeMap<int, int>::Persistent : " << n << endl;
        cout << "=============================================================" << endl;
        auto startTime = std::chrono::steady_clock::now();
        for (int i = 0; i < n; i++) {
            tree = tree.insert_v(rand_ints[i], i);
        }
        auto endTime = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsed = endTime - startTime;
        cout << elapsed.count() << 's' << endl;

        int64_t sum = 0;
        startTime = std::chrono::steady_clock::now();
        for (int i = 0; i < n; i++) {
            sum += tree[rand_ints[i]];
        }
        endTime = std::chrono::steady_clock::now();
        elapsed = endTime - startTime;
        cout << elapsed.count() << 's' << endl;
        cout << "allocations: " << allocations << " deallocations : " << deallocations << endl;
        cout << "increments: " << increments << " decrements : " << decrements << endl;
        cout << sum << endl << endl;
        reset_counters();
    }
    if constexpr (true) {
        std::map<int, int> rbtree{};
        cout << "std::map<int, int> : " << n << endl;
        cout << "=============================================================" << endl;
        auto startTime = std::chrono::steady_clock::now();
        for (int i = 0; i < n; i++) {
            rbtree[rand_ints[i]] = i;
        }
        auto endTime = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsed = endTime - startTime;
        cout << elapsed.count() << 's' << endl;

        int64_t sum = 0;
        startTime = std::chrono::steady_clock::now();
        for (int i = 0; i < n; i++) {
            sum += rbtree[rand_ints[i]];
        }
        endTime = std::chrono::steady_clock::now();
        elapsed = endTime - startTime;
        cout << elapsed.count() << 's' << endl;
        cout << sum << endl << endl;
    }
    if constexpr (true) {
        immer::flex_vector<pair<int, int>> vec{};

        cout << "immer::flex_vector<int> : " << n << endl;
        cout << "=============================================================" << endl;
        auto startTime = std::chrono::steady_clock::now();
        for (int i = 0; i < n; i++) {
            vec = vec.insert(std::lower_bound(vec.begin(), vec.end(), std::make_pair(rand_ints[i], 0)) - vec.begin(), std::make_pair(rand_ints[i], i));
        }
        auto endTime = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsed = endTime - startTime;
        cout << elapsed.count() << 's' << endl;

        int64_t sum = 0;
        startTime = std::chrono::steady_clock::now();
        for (int i = 0; i < n; i++) {
            sum += std::lower_bound(vec.begin(), vec.end(), std::make_pair(rand_ints[i], 0))->second;
        }
        endTime = std::chrono::steady_clock::now();
        elapsed = endTime - startTime;
        cout << elapsed.count() << 's' << endl;
        cout << sum << endl << endl;
    }
    if constexpr (n <= 1000*1000) {
        Vector<pair<int, int>> vec{};
        cout << "std::vector<int> : " << n << endl;
        cout << "=============================================================" << endl;
        auto startTime = std::chrono::steady_clock::now();
        for (int i = 0; i < n; i++) {
            vec.insert(std::lower_bound(vec.begin(), vec.end(), std::make_pair(rand_ints[i], 0)), std::make_pair(rand_ints[i], i));
        }
        auto endTime = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsed = endTime - startTime;
        cout << elapsed.count() << 's' << endl;

        int64_t sum = 0;
        startTime = std::chrono::steady_clock::now();
        for (int i = 0; i < n; i++) {
            sum += std::lower_bound(vec.begin(), vec.end(), std::make_pair(rand_ints[i], 0))->second;
        }
        endTime = std::chrono::steady_clock::now();
        elapsed = endTime - startTime;
        cout << elapsed.count() << 's' << endl;
        cout << sum << endl << endl;
    }
};

int main() {
    std::apply(
            [](auto... n){ (run_benchmark(n), ...); },
            std::make_tuple(
                    std::integral_constant<ssize, 1 << 23>(),
                    std::integral_constant<ssize, 1 << 21>(),
                    std::integral_constant<ssize, 1 << 19>(),
                    std::integral_constant<ssize, 1 << 17>(),
                    std::integral_constant<ssize, 1 << 15>()));
}
