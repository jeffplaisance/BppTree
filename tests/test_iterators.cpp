#include "bpptree/bpptree.hpp"
#include <chrono>
#include <iterator>
#include "gtest/gtest.h"
#include "test_common.hpp"

using namespace std;

SummedIndexedTree<uint32_t>::Transient makeTree() {
    SummedIndexedTree<uint32_t>::Transient tree{};
    auto it3 = tree.begin();
    for (uint32_t i = 1; i < 8192; ++i) {
        tree.insert(it3, i);
        ++it3;
    }
    for (uint32_t i = 16384; i >= 8192; --i) {
        tree.insert(it3, i);
    }
    auto it4 = tree.begin();
    auto it5 = tree.end();
    for (uint32_t i = 1; i <= 16384; ++i) {
        EXPECT_EQ(*it4, i);
        EXPECT_LT(it4, it5);
        ++it4;
    }
    it4 = tree.begin();
    for (uint32_t i = 1; i <= 16384; ++i) {
        *it4 = i;
        ++it4;
    }
    it4 = tree.begin();
    for (uint32_t i = 1; i <= 16384; ++i) {
        EXPECT_EQ(*it4, i);
        EXPECT_LT(it4, it5);
        ++it4;
    }
    return tree;
}

TEST(BppTreeTest, TestIterators) {
    if constexpr (true) {
        SummedIndexedTree<uint32_t>::Transient tree = makeTree();
        auto it = tree.end();
        auto it2 = tree.rbegin();
        auto rend = tree.rend();
        auto begin = tree.begin();
        EXPECT_LT(begin, it);
        EXPECT_LT(it2, rend);
        for (; it2 != rend; ++it2) {
            --it;
            EXPECT_EQ(*it, *it2);
            EXPECT_EQ(it, it2);
            EXPECT_LE(begin, it);
            EXPECT_LT(it2, rend);
        }
        EXPECT_EQ(it, begin);
    }
    if constexpr (true) {
        SummedIndexedTree<uint32_t>::Transient tree = makeTree();
        auto it = tree.begin();
        auto it2 = tree.rend();
        auto end = tree.end();
        auto rbegin = tree.rbegin();
        for (; it != end; ++it) {
            --it2;
            EXPECT_EQ(*it, *it2);
        }
        EXPECT_EQ(it2, rbegin);
    }
}
