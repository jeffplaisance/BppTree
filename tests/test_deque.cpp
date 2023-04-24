#include <deque>
#include "gtest/gtest.h"
#include "test_common.hpp"

TEST(BppTreeTest, TestDeque) {
    static constexpr int n = 10*1000*1000;
    auto rand_ints = RandInts<int32_t, n>::ints;
    std::deque<int32_t> deq{};
    using TreeType = BppTree<int32_t, 512, 512, 10>::mixins<>::Transient;
    TreeType tree{};
    for (auto i : rand_ints) {
        if (i & 1) {
            deq.emplace_front(i);
            tree.push_front(i);
        } else {
            deq.emplace_back(i);
            tree.push_back(i);
        }
    }
    EXPECT_TRUE(std::equal(deq.cbegin(), deq.cend(), tree.cbegin(), tree.cend()));
    EXPECT_EQ(deq.size(), tree.size());
    while (!deq.empty()) {
        EXPECT_EQ(deq.front(), tree.front());
        EXPECT_EQ(deq.back(), tree.back());
        EXPECT_EQ(deq.front(), tree.pop_front());
        deq.pop_front();
        EXPECT_EQ(deq.front(), tree.front());
        EXPECT_EQ(deq.back(), tree.back());
        EXPECT_EQ(deq.back(), tree.pop_back());
        deq.pop_back();
    }
    EXPECT_TRUE(tree.empty());
}

TEST(BppTreeTest, TestPersistentDeque) {
    static constexpr int n = 1000*1000;
    auto rand_ints = RandInts<int32_t, n>::ints;
    std::deque<int32_t> deq{};
    using TreeType = BppTree<int32_t, 512, 512, 8>::mixins<>::Persistent;
    TreeType tree{};
    for (auto i : rand_ints) {
        if (i & 1) {
            deq.emplace_front(i);
            tree = tree.push_front(i);
        } else {
            deq.emplace_back(i);
            tree = tree.push_back(i);
        }
    }
    EXPECT_TRUE(std::equal(deq.cbegin(), deq.cend(), tree.cbegin(), tree.cend()));
    EXPECT_EQ(deq.size(), tree.size());
    while (!deq.empty()) {
        EXPECT_EQ(deq.front(), tree.front());
        EXPECT_EQ(deq.back(), tree.back());
        tree = tree.pop_front();
        deq.pop_front();
        EXPECT_EQ(deq.front(), tree.front());
        EXPECT_EQ(deq.back(), tree.back());
        tree = tree.pop_back();
        deq.pop_back();
    }
    EXPECT_TRUE(tree.empty());
}
