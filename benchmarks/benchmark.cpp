#include <iostream>
#include <chrono>
#include <map>
#include "test_common.hpp"
#include "immer/flex_vector.hpp"
#include "absl/container/btree_map.h"
#include "tlx/container/btree_map.hpp"

using namespace std;

template <auto n, typename T>
inline void random_benchmark(T&& tree, std::string const& message) {
    Vector<int32_t> const& rand_ints = RandInts<int32_t, n>::ints;
    cout << "Running random benchmark using " << message << " with size " << n << endl;
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
    cout << sum << endl << endl;
}

template <typename N, auto n>
inline void random_benchmarks(std::integral_constant<N, n>) {
    Vector<int32_t> const& rand_ints = RandInts<int32_t, n>::ints;
    for (int j = 0; j < 5; ++j) {
        random_benchmark<n>(absl::btree_map<int, int>(), "absl::btree_map<int, int>");
        random_benchmark<n>(tlx::btree_map<int, int>(), "tlx::btree_map<int, int>");
        random_benchmark<n>(BppTreeMap<int, int>::Transient(), "BppTreeMap<int, int>::Transient");
        random_benchmark<n>(std::map<int, int>(), "std::map<int, int>");
        if constexpr (true) {
            BppTreeMap<int, int>::Transient tree{};
            cout << "BppTreeMap<int, int>::Transient (no operators)" << " : " << n << endl;
            cout << "=============================================================" << endl;
            auto startTime = std::chrono::steady_clock::now();
            for (int i = 0; i < n; i++) {
                tree.insert_or_assign(rand_ints[i], i);
            }
            auto endTime = std::chrono::steady_clock::now();
            std::chrono::duration<double> elapsed = endTime - startTime;
            cout << elapsed.count() << 's' << endl;

            int64_t sum = 0;
            startTime = std::chrono::steady_clock::now();
            for (int i = 0; i < n; i++) {
                sum += tree.at_key(rand_ints[i]);
            }
            endTime = std::chrono::steady_clock::now();
            elapsed = endTime - startTime;
            cout << elapsed.count() << 's' << endl;
            cout << sum << endl << endl;
        }
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
}

template <auto n, typename T>
inline void sequential_benchmark(T&& tree, std::string const& message) {
    cout << "Running sequential benchmark using " << message << " with size " << n << endl;
    cout << "=============================================================" << endl;
    auto startTime = std::chrono::steady_clock::now();
    for (int i = 0; i < n; i++) {
        tree[i] = i;
    }
    auto endTime = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsed = endTime - startTime;
    cout << elapsed.count() << 's' << endl;

    int64_t sum = 0;
    startTime = std::chrono::steady_clock::now();
    for (int i = 0; i < n; i++) {
        sum += tree[i];
    }
    endTime = std::chrono::steady_clock::now();
    elapsed = endTime - startTime;
    cout << elapsed.count() << 's' << endl;
    cout << sum << endl << endl;
}

template <typename N, auto n>
inline void sequential_benchmarks(std::integral_constant<N, n>) {
    for (int j = 0; j < 5; ++j) {
        sequential_benchmark<n>(absl::btree_map<int, int>(), "absl::btree_map<int, int>");
        sequential_benchmark<n>(tlx::btree_map<int, int>(), "tlx::btree_map<int, int>");
        sequential_benchmark<n>(BppTreeMap<int, int>::Transient(), "BppTreeMap<int, int>::Transient");
        sequential_benchmark<n>(std::map<int, int>(), "std::map<int, int>");
    }
    if constexpr (true) {
        using TreeType = BppTreeMap<int, int>::internal_node_bytes<256>::leaf_node_bytes<1024>::Persistent;
        TreeType tree{};
        cout << "BppTreeMap<int, int>::Persistent : " << n << endl;
        cout << "=============================================================" << endl;
        auto startTime = std::chrono::steady_clock::now();
        for (int i = 0; i < n; i++) {
            tree = tree.insert_v(i, i);
        }
        auto endTime = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsed = endTime - startTime;
        cout << elapsed.count() << 's' << endl;

        int64_t sum = 0;
        startTime = std::chrono::steady_clock::now();
        for (int i = 0; i < n; i++) {
            sum += tree[i];
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
            vec = vec.insert(std::lower_bound(vec.begin(), vec.end(), std::make_pair(i, 0)) - vec.begin(), std::make_pair(i, i));
        }
        auto endTime = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsed = endTime - startTime;
        cout << elapsed.count() << 's' << endl;

        int64_t sum = 0;
        startTime = std::chrono::steady_clock::now();
        for (int i = 0; i < n; i++) {
            sum += std::lower_bound(vec.begin(), vec.end(), std::make_pair(i, 0))->second;
        }
        endTime = std::chrono::steady_clock::now();
        elapsed = endTime - startTime;
        cout << elapsed.count() << 's' << endl;
        cout << sum << endl << endl;
    }
    if constexpr (true) {
        Vector<pair<int, int>> vec{};
        cout << "std::vector<int> : " << n << endl;
        cout << "=============================================================" << endl;
        auto startTime = std::chrono::steady_clock::now();
        for (int i = 0; i < n; i++) {
            vec.insert(std::lower_bound(vec.begin(), vec.end(), std::make_pair(i, 0)), std::make_pair(i, i));
        }
        auto endTime = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsed = endTime - startTime;
        cout << elapsed.count() << 's' << endl;

        int64_t sum = 0;
        startTime = std::chrono::steady_clock::now();
        for (int i = 0; i < n; i++) {
            sum += std::lower_bound(vec.begin(), vec.end(), std::make_pair(i, 0))->second;
        }
        endTime = std::chrono::steady_clock::now();
        elapsed = endTime - startTime;
        cout << elapsed.count() << 's' << endl;
        cout << sum << endl << endl;
    }
}

int main() {
    auto sizes = std::make_tuple(
            std::integral_constant<ssize, 1 << 23>(),
            std::integral_constant<ssize, 1 << 21>(),
            std::integral_constant<ssize, 1 << 19>(),
            std::integral_constant<ssize, 1 << 17>(),
            std::integral_constant<ssize, 1 << 15>());
    std::apply([](auto... n){ (sequential_benchmarks(n), ...); }, sizes);
    std::apply([](auto... n){ (random_benchmarks(n), ...); }, sizes);
}
