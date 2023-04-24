#include <map>
#include <iterator>
#include "gtest/gtest.h"

#include "test_common.hpp"

TEST(BppTreeTest, TestPersistentRandomModificationsIndexedIterator) {
    size_t n = 1000*1000;
    using TreeType = BppTree<size_t, 512, 512, 6>::mixins2<Indexed>::Persistent;
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
    using TreeType = BppTree<size_t, 512, 512, 6>::mixins2<Indexed>::Persistent;
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

TEST(BppTreeTest, TestPersistentRandomModificationsOrdered) {
    size_t n = 1000*1000;
    using TreeType = BppTree<std::pair<size_t, size_t>, 512, 512, 6>::mixins2<Ordered, Indexed>::Persistent;
    TreeType tree{};
    std::map<size_t, size_t> map{};
    for (size_t i = 0; i < n; ++i) {
        double d = static_cast<double>(rand())/RAND_MAX;
        if (tree.size() == 0 || d < 0.4) {
            auto r = static_cast<size_t>(rand());
            if (tree.contains(r) || d < 0.2) {
                tree = tree.insert_or_assign(std::piecewise_construct, std::forward_as_tuple(r), std::forward_as_tuple(i));
                map[r] = i;
            } else {
                tree = tree.insert_v(r, i);
                map[r] = i;
            }
        } else {
            size_t index = static_cast<size_t>(rand()) % tree.size();
            size_t key = tree.at_index(index).first;
            if (d < 0.6) {
                tree = tree.assign_v(key, i);
                map[key] = i;
            } else if (d < 0.8) {
                tree = tree.update_key(key, [](auto const& v){ return v + 1; });
                ++map[key];
            } else {
                tree = tree.erase_key(key);
                map.erase(key);
            }
        }
    }
    EXPECT_TRUE(std::equal(
            map.cbegin(),
            map.cend(),
            tree.cbegin(),
            tree.cend(),
            [](auto const& a, auto const& b){ return a.first == b.first && a.second == b.second; }));
}

TEST(BppTreeTest, TestPersistentRandomModificationsOrderedIterator) {
    size_t n = 1000*1000;
    using TreeType = BppTree<std::pair<size_t, size_t>, 512, 512, 6>::mixins2<Ordered, Indexed>::Persistent;
    TreeType tree{};
    std::map<size_t, size_t> map{};
    for (size_t i = 0; i < n; ++i) {
        double d = static_cast<double>(rand())/RAND_MAX;
        if (tree.size() == 0 || d < 0.4) {
            auto r = static_cast<size_t>(rand()) % 1000000;
            if (tree.contains(r)) {
                auto it = std::as_const(tree).lower_bound(r);
                tree = tree.assign(it, std::make_pair(r, i));
                map[r] = i;
            } else {
                auto it = tree.lower_bound(r);
                tree = tree.insert(it, std::make_pair(r, i));
                map[r] = i;
            }
        } else {
            size_t index = static_cast<size_t>(rand()) % tree.size();
            size_t key = tree.at_index(index).first;
            if (d < 0.6) {
                auto it = tree.find(key);
                tree = tree.assign(it, std::make_pair(key, i));
                map[key] = i;
            } else if (d < 0.8) {
                auto it = tree.find_const(key);
                tree = tree.update(it, [](auto const& p){ return std::make_pair(p.first, p.second + 1); });
                ++map[key];
            } else {
                auto it = std::as_const(tree).find(key);
                tree = tree.erase(it);
                map.erase(key);
            }
        }
    }
    EXPECT_TRUE(std::equal(
            map.cbegin(),
            map.cend(),
            tree.cbegin(),
            tree.cend(),
            [](auto const& a, auto const& b){ return a.first == b.first && a.second == b.second; }));
}

TEST(BppTreeTest, TestTransientRandomModificationsOrdered) {
    size_t n = 1000*1000;
    using TreeType = BppTree<std::tuple<size_t, size_t>, 512, 512, 6>::mixins<OrderedBuilder<>::extractor<TupleExtractor<0>>, IndexedBuilder<>>::Transient;
    TreeType tree{};
    std::map<size_t, size_t> map{};
    for (size_t i = 0; i < n; ++i) {
        double d = static_cast<double>(rand())/RAND_MAX;
        if (tree.size() == 0 || d < 0.4) {
            auto r = static_cast<size_t>(rand());
            tree[r] = i;
            map[r] = i;
        } else {
            size_t index = static_cast<size_t>(rand()) % tree.size();
            size_t key = std::get<0>(tree.at_index(index));
            if (d < 0.6) {
                tree[key] = i;
                map[key] = i;
            } else if (d < 0.8) {
                ++tree[key];
                ++map[key];
            } else {
                tree.erase_key(key);
                map.erase(key);
            }
        }
    }
    EXPECT_TRUE(std::equal(
            map.cbegin(),
            map.cend(),
            tree.cbegin(),
            tree.cend(),
            [](auto const& a, auto const& b){ return a.first == std::get<0>(b) && a.second == std::get<1>(b); }));
}
