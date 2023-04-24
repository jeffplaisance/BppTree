#include <iostream>
#include <chrono>
#include <vector>
#include <iterator>
#include "gtest/gtest.h"
#include "test_common.hpp"

using namespace std;

template <bool binary_search>
void test_ordered_transient() {
    static constexpr int n = 1000*1000;

    safe_vector<int32_t> const& rand_ints = RandInts<int32_t, n>::ints;
    typename OrderedTree<std::pair<int, int>, PairExtractor<0>, int64_t, PairExtractor<1>, MinComparator, binary_search>::Transient tree{};
    int start = 0;
    int end = n - 1;
    auto startTime = std::chrono::steady_clock::now();
    for (int i = 0; i < n / 2; i++) {
        tree[end] = rand_ints[end];
        tree[start] = std::make_pair(start, rand_ints[start]);
        ++start;
        --end;
    }
    auto endTime = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsed = endTime - startTime;
    cout << elapsed.count() << 's' << endl;
    auto sorted_rand_ints = rand_ints;
    std::sort(sorted_rand_ints.begin(), sorted_rand_ints.end());
    int64_t sum = 0;
    startTime = std::chrono::steady_clock::now();
    for (int i = 0; i < n; i++) {
        sum += tree.at_key(sorted_rand_ints[i]%n);
        auto it = tree.find(sorted_rand_ints[i]%n);
        EXPECT_EQ(sum, tree.sum_inclusive(it));
    }
    endTime = std::chrono::steady_clock::now();
    elapsed = endTime - startTime;
    cout << elapsed.count() << 's' << endl;
    cout << sum << endl;
    cout << tree.sum() << endl;
    cout << "size: " << tree.size() << ", n: " << n << endl;
    cout << "inserting elements again" << endl;
    start = 0;
    end = n - 1;
    for (int i = 0; i < n / 2; i++) {
        auto end_pair = std::make_pair(end, rand_ints[end]);
        tree[end] = end_pair;
        auto const start_pair = std::make_pair(start, rand_ints[start]);
        tree[start] = start_pair;
        ++start;
        --end;
    }
    cout << "size: " << tree.size() << ", n: " << n << endl;
    cout << "deleting every other element from ordered tree" << endl;
    sorted_rand_ints.clear();
    for (int i = 0; i < n; i++) {
        if (i % 2 == 0) {
            tree.erase_key(rand_ints[i] % n);
        } else {
            sorted_rand_ints.push_back(rand_ints[i] % n);
        }
    }
    sort(sorted_rand_ints.begin(), sorted_rand_ints.end());
    cout << "size: " << tree.size() << endl;
    sum = 0;
    for (size_t i = 0; i < sorted_rand_ints.size(); i++) {
        sum += std::as_const(tree)[sorted_rand_ints[i] % n];
        auto it = tree.find(sorted_rand_ints[i] % n);
        EXPECT_EQ(it.get().second, tree.at_key(sorted_rand_ints[i] % n));
        EXPECT_EQ(sum, tree.sum_inclusive(it));
    }
    EXPECT_EQ(sorted_rand_ints.size(), tree.size());
    for (auto i : sorted_rand_ints) {
        int val = tree[i];
        ASSERT_EQ(val, tree[i]++);
    }
    EXPECT_EQ(sum+signed_cast(tree.size()), tree.sum());
    for (auto i : sorted_rand_ints) {
        int val = tree[i];
        ASSERT_EQ(val, tree[i]--);
    }
    EXPECT_EQ(sum, tree.sum());
    for (auto i : sorted_rand_ints) {
        ++tree[i];
    }
    EXPECT_EQ(sum+signed_cast(tree.size()), tree.sum());
    for (auto i : sorted_rand_ints) {
        --tree[i];
    }
    EXPECT_EQ(sum, tree.sum());
    for (auto i : sorted_rand_ints) {
        tree[i]+=1;
    }
    EXPECT_EQ(sum+signed_cast(tree.size()), tree.sum());
    for (auto i : sorted_rand_ints) {
        tree[i]-=1;
    }
    EXPECT_EQ(sum, tree.sum());
}

TEST(BppTreeTest, TestOrderedTransientLinearSearch) {
    test_ordered_transient<false>();
}

TEST(BppTreeTest, TestOrderedTransientBinarySearch) {
    test_ordered_transient<true>();
}

template <bool binary_search>
void test_ordered_persistent() {
    static constexpr int n = 100*1000;

    safe_vector<int32_t> const& rand_ints = RandInts<int32_t, n>::ints;
    typename OrderedTree<std::pair<int, int>, PairExtractor<0>, int64_t, PairExtractor<1>, MinComparator, binary_search>::Persistent tree{};
    int start = 0;
    int end = n - 1;
    auto startTime = std::chrono::steady_clock::now();
    for (int i = 0; i < n / 2; i++) {
        tree = tree.insert_or_assign(end, rand_ints[end]);
        tree = tree.insert_or_assign(start, rand_ints[start]);
        ++start;
        --end;
    }
    auto endTime = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsed = endTime - startTime;
    cout << elapsed.count() << 's' << endl;
    auto sorted_rand_ints = rand_ints;
    std::sort(sorted_rand_ints.begin(), sorted_rand_ints.end());
    int64_t sum = 0;
    startTime = std::chrono::steady_clock::now();
    for (int i = 0; i < n; i++) {
        sum += tree.at_key(sorted_rand_ints[i]%n);
        auto it = tree.find(sorted_rand_ints[i]%n);
        EXPECT_EQ(sum, tree.sum_inclusive(it));
    }
    endTime = std::chrono::steady_clock::now();
    elapsed = endTime - startTime;
    cout << elapsed.count() << 's' << endl;
    cout << sum << endl;
    cout << tree.sum() << endl;
    cout << "size: " << tree.size() << ", n: " << n << endl;
    cout << "inserting elements again" << endl;
    start = 0;
    end = n - 1;
    for (int i = 0; i < n / 2; i++) {
        tree = tree.insert_or_assign(end, rand_ints[end]);
        tree = tree.insert_or_assign(start, rand_ints[start]);
        ++start;
        --end;
    }
    cout << "size: " << tree.size() << ", n: " << n << endl;
    cout << "deleting every other element from ordered tree" << endl;
    sorted_rand_ints.clear();
    for (int i = 0; i < n; i++) {
        if (i % 2 == 0) {
            tree = tree.erase_key(rand_ints[i] % n);
        } else {
            sorted_rand_ints.push_back(rand_ints[i] % n);
        }
    }
    sort(sorted_rand_ints.begin(), sorted_rand_ints.end());
    cout << "size: " << tree.size() << endl;
    sum = 0;
    for (size_t i = 0; i < sorted_rand_ints.size(); i++) {
        sum += tree.at_key(sorted_rand_ints[i] % n);
        auto it = tree.find(sorted_rand_ints[i] % n);
        EXPECT_EQ(it.get().second, tree.at_key(sorted_rand_ints[i] % n));
        EXPECT_EQ(sum, tree.sum_inclusive(it));
    }
    EXPECT_EQ(sorted_rand_ints.size(), tree.size());
}

TEST(BppTreeTest, TestOrderedPersistentLinearSearch) {
    test_ordered_persistent<false>();
}

TEST(BppTreeTest, TestOrderedPersistentBinarySearch) {
    test_ordered_persistent<true>();
}

TEST(BppTreeTest, TestOrderedTransientSet) {
    static constexpr int n = 100*1000;
    safe_vector<int32_t> const& rand_ints = RandInts<int32_t, n>::ints;

    using TreeType = BppTree<int32_t, 512, 512, 6>::mixins<OrderedBuilder<>::extractor<ValueExtractor>>::Transient;
    TreeType tree{};
    std::set<int32_t> set{};
    for (int32_t i : rand_ints) {
        tree.insert_or_assign(i);
        set.insert(i);
    }
    EXPECT_EQ(tree.size(), set.size());
    for (int32_t i : set) {
        EXPECT_TRUE(tree.contains(i));
    }
    for (int32_t i : tree) {
        EXPECT_TRUE(set.find(i) != set.end());
    }
    for (int32_t i : rand_ints) {
        tree.insert_or_assign(i);
        set.insert(i);
    }
    EXPECT_EQ(tree.size(), set.size());
    for (int32_t i : set) {
        EXPECT_TRUE(tree.contains(i));
    }
    for (int32_t i : tree) {
        EXPECT_TRUE(set.find(i) != set.end());
    }
}
