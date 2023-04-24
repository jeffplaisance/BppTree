#include <map>
#include <iterator>
#include "gtest/gtest.h"

#include "test_common.hpp"

TEST(BppTreeTest, TestPersistentRandomModificationsIndexedIterator) {
    size_t n = 1000*1000;
    using TreeType = BppTree<size_t, 512, 512, 5>::mixins2<Indexed>::Persistent;
    TreeType tree{};
    std::vector<size_t> vec{};
    for (size_t i = 0; i < n; ++i) {
        double d = static_cast<double>(rand())/RAND_MAX;
        size_t index = static_cast<size_t>(rand()) % (tree.size() + 1);
        auto vec_it = vec.begin() + signed_cast(index);
        if (tree.size() == 0 || index == tree.size() || d < 0.4) {
            auto it = tree.find_index_const(index);
            tree = tree.insert(it, i);
            vec.insert(vec_it, i);
        } else if (d < 0.6) {
            auto it = std::as_const(tree).find_index(index);
            tree = tree.assign(it, i);
            *vec_it = i;
        } else if (d < 0.8) {
            auto it = std::as_const(tree).find_index_const(index);
            tree = tree.update(it, [](auto const& v){ return v + 1; });
            ++*vec_it;
        } else {
            auto it = tree.find_index(index);
            tree = tree.erase(it);
            vec.erase(vec_it);
        }
    }
    EXPECT_TRUE(std::equal(vec.begin(), vec.end(), tree.begin(), tree.end()));
}

TEST(BppTreeTest, TestPersistentRandomModificationsIndexed) {
    size_t n = 1000*1000;
    using TreeType = BppTree<size_t, 512, 512, 5>::mixins2<Indexed>::Persistent;
    TreeType tree{};
    std::vector<size_t> vec{};
    for (size_t i = 0; i < n; ++i) {
        double d = static_cast<double>(rand())/RAND_MAX;
        size_t index = static_cast<size_t>(rand()) % (tree.size() + 1);
        auto vec_it = vec.begin() + signed_cast(index);
        if (tree.size() == 0 || index == tree.size() || d < 0.4) {
            tree = tree.insert_index(index, i);
            vec.insert(vec_it, i);
        } else if (d < 0.6) {
            tree = tree.assign_index(index, i);
            *vec_it = i;
        } else if (d < 0.8) {
            tree = tree.update_index(index, [](auto const& v){ return v + 1; });
            ++*vec_it;
        } else {
            tree = tree.erase_index(index);
            vec.erase(vec_it);
        }
    }
    EXPECT_TRUE(std::equal(vec.begin(), vec.end(), tree.begin(), tree.end()));
}
