#include "bpptree/bpptree.hpp"
#include "bpptree/indexed.hpp"
#include "bpptree/min.hpp"
#include "bpptree/max.hpp"
#include <chrono>
#include <vector>
#include <iterator>
#include "gtest/gtest.h"
#include "test_common.hpp"

using namespace std;
using namespace bpptree::detail;

template <typename TreeType>
static void do_stuff(TreeType& tree, Vector<uint32_t>& vec) {
    for (uint32_t i = 0; i < tree.size(); ++i) {
        pair<uint32_t, uint32_t> t = tree[i];
        EXPECT_EQ(vec[i], std::get<0>(t));
    }
    for (uint32_t i = 0; i < tree.size(); ++i) {
        for (uint32_t j = i + 1; j <= tree.size(); ++j) {
            auto vec_it = std::min_element(vec.begin()+i, vec.begin()+j);
            const auto tree_begin = tree.begin() + i;
            const auto tree_end = tree.begin() + j;
            EXPECT_LT(tree_begin, tree_end);
            EXPECT_FALSE(tree_end < tree_begin);
            EXPECT_LE(tree_begin, tree_end);
            EXPECT_FALSE(tree_end <= tree_begin);
            EXPECT_GT(tree_end, tree_begin);
            EXPECT_FALSE(tree_begin > tree_end);
            EXPECT_GE(tree_end, tree_begin);
            EXPECT_FALSE(tree_begin >= tree_end);
            EXPECT_EQ(tree_begin+(j-i), tree_end);
            EXPECT_FALSE(tree_begin+(j-i) != tree_end);
            EXPECT_LE(tree_begin+(j-i), tree_end);
            EXPECT_LE(tree_end, tree_begin+(j-i));
            EXPECT_GE(tree_begin+(j-i), tree_end);
            EXPECT_GE(tree_end, tree_begin+(j-i));
            EXPECT_FALSE(tree_begin+(j-i) < tree_end);
            EXPECT_FALSE(tree_end < tree_begin+(j-i));
            EXPECT_FALSE(tree_begin+(j-i) > tree_end);
            EXPECT_FALSE(tree_end > tree_begin+(j-i));
            EXPECT_EQ(tree_end - tree_begin, j - i);
            auto min = tree.min(tree_begin, tree_end);
            auto tree_it = tree.min_element(tree_begin, tree_end);
            EXPECT_FALSE(min != std::get<0>(tree_it.get()) || min != *vec_it);
        }
    }
    auto vec_it = std::min_element(vec.begin(), vec.end());
    auto min = tree.min();
    auto tree_it = tree.min_element();
    EXPECT_FALSE(min != std::get<0>(tree_it.get()) || min != *vec_it);
}

TEST(BppTreeTest, TestMinTransient) {
    using TreeType = BppTree<pair<uint32_t, uint32_t>, 128, 128, 4>::mixins<IndexedBuilder<>, MinBuilder<>::extractor<PairExtractor<0>>>;
    TreeType::Transient tree{};
    Vector<uint32_t> vec{};
    for (uint32_t i = 0; i < 1024; ++i) {
        auto r = static_cast<uint32_t>(rand());
        tree.emplace_back(r, i);
        vec.emplace_back(r);
    }
    do_stuff(tree, vec);
    auto tree_copy = tree.persistent();
    for (size_t i = 0; i < 512; ++i) {
        tree.erase_index(i);
        vec.erase(vec.begin()+signed_cast(i));
    }
    do_stuff(tree, vec);
    tree = tree_copy.transient();
    std::sort(tree.begin(),tree.end());
    EXPECT_TRUE(std::is_sorted(tree.begin(), tree.end()));
    for (uint32_t i = 0; i < tree.size(); ++i) {
        EXPECT_TRUE(*tree.min_element_const(tree.begin()+i, tree.end()) == tree[i]);
    }
}

TEST(BppTreeTest, TestMinPersistent) {
    using TreeType = BppTree<pair<uint32_t, uint32_t>, 128, 128, 4>::mixins<IndexedBuilder<>, MinBuilder<>::extractor<PairExtractor<0>>>;
    TreeType::Persistent tree{};
    Vector<uint32_t> vec{};
    for (uint32_t i = 0; i < 1024; ++i) {
        auto r = static_cast<uint32_t>(rand());
        tree = tree.emplace_back(r, i);
        vec.emplace_back(r);
    }
    do_stuff(tree, vec);
    auto tree_copy = tree;
    for (size_t i = 0; i < 512; ++i) {
        tree = tree.erase_index(i);
        vec.erase(vec.begin()+signed_cast(i));
    }
    do_stuff(tree, vec);
    TreeType::Transient transient = tree_copy.transient();
    std::sort(transient.begin(),transient.end());
    tree = transient.persistent();
    EXPECT_TRUE(std::is_sorted(tree.begin(), tree.end()));
    for (uint32_t i = 0; i < tree.size(); ++i) {
        EXPECT_EQ(*tree.min_element_const(tree.begin()+i, tree.end()), tree[i]);
    }
}

TEST(BppTreeTest, TestMaxTransient) {
    using TreeType = BppTree<pair<uint32_t, uint32_t>, 128, 128, 4>::mixins<IndexedBuilder<>, MaxBuilder<>::extractor<PairExtractor<0>>>;
    TreeType::Transient tree{};
    Vector<uint32_t> vec{};
    for (uint32_t i = 0; i < 1024; ++i) {
        auto r = static_cast<uint32_t>(rand());
        tree.emplace_back(r, i);
        vec.emplace_back(r);
    }
    for (uint32_t i = 0; i < tree.size(); ++i) {
        pair<uint32_t, uint32_t> t = tree[i];
        EXPECT_EQ(vec[i], std::get<0>(t));
    }
    for (uint32_t i = 0; i < tree.size(); ++i) {
        for (uint32_t j = i + 1; j <= tree.size(); ++j) {
            auto vec_it = std::max_element(vec.begin()+i, vec.begin()+j);
            const auto tree_begin = tree.begin() + i;
            const auto tree_end = tree.begin() + j;
            EXPECT_LT(tree_begin, tree_end);
            EXPECT_FALSE(tree_end < tree_begin);
            EXPECT_LE(tree_begin, tree_end);
            EXPECT_FALSE(tree_end <= tree_begin);
            EXPECT_GT(tree_end, tree_begin);
            EXPECT_FALSE(tree_begin > tree_end);
            EXPECT_GE(tree_end, tree_begin);
            EXPECT_FALSE(tree_begin >= tree_end);
            EXPECT_EQ(tree_begin+(j-i), tree_end);
            EXPECT_FALSE(tree_begin+(j-i) != tree_end);
            EXPECT_LE(tree_begin+(j-i), tree_end);
            EXPECT_LE(tree_end, tree_begin+(j-i));
            EXPECT_GE(tree_begin+(j-i), tree_end);
            EXPECT_GE(tree_end, tree_begin+(j-i));
            EXPECT_FALSE(tree_begin+(j-i) < tree_end);
            EXPECT_FALSE(tree_end < tree_begin+(j-i));
            EXPECT_FALSE(tree_begin+(j-i) > tree_end);
            EXPECT_FALSE(tree_end > tree_begin+(j-i));
            EXPECT_EQ(tree_end - tree_begin, j - i);
            auto max = tree.max(tree_begin, tree_end);
            auto tree_it = tree.max_element(tree_begin, tree_end);
            EXPECT_EQ(max, tree_it->first);
            ASSERT_EQ(max, *vec_it);
        }
    }
    auto vec_it = std::max_element(vec.begin(), vec.end());
    auto max = tree.max();
    auto tree_it = tree.max_element();
    EXPECT_FALSE(max != std::get<0>(tree_it.get()) || max != *vec_it);
    std::sort(tree.begin(), tree.end(), [](pair<uint32_t, uint32_t> const& a, pair<uint32_t, uint32_t> const& b){ return b < a; });
    EXPECT_TRUE(std::is_sorted(tree.begin(), tree.end(), [](pair<uint32_t, uint32_t> const& a, pair<uint32_t, uint32_t> const& b){ return b < a; }));
    for (uint32_t i = 0; i < tree.size(); ++i) {
        EXPECT_EQ(*tree.max_element_const(tree.begin()+i, tree.end()), tree[i].get());
    }
}

TEST(BppTreeTest, TestMaxPersistent) {
    using TreeType = BppTree<pair<uint32_t, uint32_t>, 128, 128, 4>::mixins<IndexedBuilder<>, MaxBuilder<>::extractor<PairExtractor<0>>>;
    TreeType::Persistent tree{};
    Vector<uint32_t> vec{};
    for (uint32_t i = 0; i < 1024; ++i) {
        auto r = static_cast<uint32_t>(rand());
        tree = tree.emplace_back(r, i);
        vec.emplace_back(r);
    }
    for (uint32_t i = 0; i < tree.size(); ++i) {
        pair<uint32_t, uint32_t> t = tree[i];
        EXPECT_EQ(vec[i], std::get<0>(t));
    }
    for (uint32_t i = 0; i < tree.size(); ++i) {
        for (uint32_t j = i + 1; j <= tree.size(); ++j) {
            auto vec_it = std::max_element(vec.begin()+i, vec.begin()+j);
            const auto tree_begin = tree.begin() + i;
            const auto tree_end = tree.begin() + j;
            EXPECT_LT(tree_begin, tree_end);
            EXPECT_FALSE(tree_end < tree_begin);
            EXPECT_LE(tree_begin, tree_end);
            EXPECT_FALSE(tree_end <= tree_begin);
            EXPECT_GT(tree_end, tree_begin);
            EXPECT_FALSE(tree_begin > tree_end);
            EXPECT_GE(tree_end, tree_begin);
            EXPECT_FALSE(tree_begin >= tree_end);
            EXPECT_EQ(tree_begin+(j-i), tree_end);
            EXPECT_FALSE(tree_begin+(j-i) != tree_end);
            EXPECT_LE(tree_begin+(j-i), tree_end);
            EXPECT_LE(tree_end, tree_begin+(j-i));
            EXPECT_GE(tree_begin+(j-i), tree_end);
            EXPECT_GE(tree_end, tree_begin+(j-i));
            EXPECT_FALSE(tree_begin+(j-i) < tree_end);
            EXPECT_FALSE(tree_end < tree_begin+(j-i));
            EXPECT_FALSE(tree_begin+(j-i) > tree_end);
            EXPECT_FALSE(tree_end > tree_begin+(j-i));
            EXPECT_EQ(tree_end - tree_begin, j - i);
            auto max = tree.max(tree_begin, tree_end);
            auto tree_it = tree.max_element(tree_begin, tree_end);
            EXPECT_FALSE(max != std::get<0>(tree_it.get()) || max != *vec_it);
        }
    }
    auto vec_it = std::max_element(vec.begin(), vec.end());
    auto max = tree.max();
    auto tree_it = tree.max_element();
    EXPECT_FALSE(max != std::get<0>(tree_it.get()) || max != *vec_it);
    TreeType::Transient transient = tree.transient();
    std::sort(transient.begin(),transient.end(), [](pair<uint32_t, uint32_t> const& a, pair<uint32_t, uint32_t> const& b){ return b < a; });
    tree = transient.persistent();
    EXPECT_TRUE(std::is_sorted(tree.begin(), tree.end(), [](pair<uint32_t, uint32_t> const& a, pair<uint32_t, uint32_t> const& b){ return b < a; }));
    for (uint32_t i = 0; i < tree.size(); ++i) {
        EXPECT_EQ(*tree.max_element_const(tree.begin()+i, tree.end()), tree[i]);
    }
}
