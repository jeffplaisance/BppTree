#include "gtest/gtest.h"
#include "test_common.hpp"

template <bool reverse, typename T, typename B, typename E>
static void run_test(T& tree, B&& begin_f, E&& end_f) {
    static constexpr int n = 1000*1000;
    auto const& rand_ints = RandInts<int32_t, n>::ints;
    auto begin = begin_f();
    for (int32_t i : rand_ints) {
        tree.insert(begin, i);
        EXPECT_EQ(*begin, i);
        EXPECT_EQ(reverse? tree.back() : tree.front(), i);
    }
    auto end = end_f();
    for (int32_t i : rand_ints) {
        tree.insert(end, i);
        EXPECT_EQ(*end, i);
        EXPECT_EQ(reverse ? tree.front() : tree.back(), i);
        ++end;
        EXPECT_EQ(end, end_f());
    }
    while (!tree.empty()) {
        --end;
        begin = begin_f();
        EXPECT_EQ(tree.front(), tree.back());
        EXPECT_EQ(*begin, *end);
        //invalidates end
        tree.erase(begin);
        EXPECT_EQ(begin, begin_f());
        end = end_f() - 1;
        //invalidates begin
        tree.erase(end);
        EXPECT_EQ(end, end_f());
    }
    begin = begin_f();
    for (int32_t i : rand_ints) {
        tree.insert(begin, i);
        EXPECT_EQ(*begin, i);
        EXPECT_EQ(reverse? tree.back() : tree.front(), i);
    }
    end = end_f();
    for (int32_t i : rand_ints) {
        tree.insert(end, i);
        EXPECT_EQ(*end, i);
        EXPECT_EQ(reverse ? tree.front() : tree.back(), i);
        ++end;
        EXPECT_EQ(end, end_f());
    }
    auto middle = begin_f() + signed_cast(rand_ints.size());
    while (!tree.empty()) {
        --middle;
        auto i = *middle;
        ASSERT_EQ(i, middle[1]);
        tree.erase(middle);
        ASSERT_EQ(i, *middle);
        tree.erase(middle);
    }
    static constexpr int32_t m = 100*1000;
    auto rand_ints2 = RandInts<uint32_t, m>::ints;
    Vector<int32_t> vec{};
    for (int32_t i = 0; i < m; ++i) {
        size_t index = rand_ints2[i] % (tree.size() + 1);
        if constexpr (!reverse) {
            auto it = tree.find_index(index);
            tree.insert(it, i);
            vec.insert(vec.begin() + signed_cast(index), i);
            EXPECT_EQ(*it, vec[index]);
        } else {
            auto it = tree.rbegin() + signed_cast(index);
            tree.insert(it, i);
            size_t vec_index = vec.size() - index;
            vec.insert(vec.begin() + signed_cast(vec_index), i);
            EXPECT_EQ(*it, vec[vec_index]);
        }
    }
    EXPECT_TRUE(std::equal(vec.cbegin(), vec.cend(), tree.cbegin(), tree.cend()));
    EXPECT_TRUE(std::equal(vec.crbegin(), vec.crend(), tree.crbegin(), tree.crend()));
}

TEST(BppTreeTest, TestIteratorInsertErase) {
    using TreeType = BppTree<int32_t, 512, 512, 6>::mixins<IndexedBuilder<>>;
    TreeType::Transient tree{};
    run_test<false>(tree, [&tree]() { return tree.begin(); }, [&tree]() { return tree.end(); });
}

TEST(BppTreeTest, TestIteratorInsertEraseReverse) {
    using TreeType = BppTree<int32_t, 512, 512, 6>::mixins<IndexedBuilder<>>;
    TreeType::Transient tree{};
    run_test<true>(tree, [&tree]() { return tree.rbegin(); }, [&tree]() { return tree.rend(); });
}
