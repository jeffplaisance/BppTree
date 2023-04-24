#include "gtest/gtest.h"
#include "test_common.hpp"

using namespace std;

TEST(BppTreeTest, TestSumLowerBound) {
    using TreeType = BppTree<uint32_t>::mixins<SummedBuilder<>>::Transient;
    TreeType tree{};
    for (uint32_t i = 1; i <= 1024; ++i) {
        tree.push_back(i);
    }
    uint32_t sum = 0;
    auto end = tree.end();
    for (auto it = tree.begin(); it < end; ++it) {
        EXPECT_EQ(sum, tree.sum_exclusive(it));
        if (sum > 0) {
            EXPECT_EQ(sum, tree.sum_inclusive(tree.sum_lower_bound(sum)));
        }
        sum += *it;
        EXPECT_EQ(sum, tree.sum_inclusive(it));
        EXPECT_EQ(sum, tree.sum_inclusive(tree.sum_lower_bound(sum)));
    }
    EXPECT_EQ(sum, tree.sum_exclusive(tree.end()));
}
