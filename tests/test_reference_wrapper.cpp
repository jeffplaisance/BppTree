#include "bpptree/bpptree.hpp"
#include "bpptree/min.hpp"
#include <chrono>
#include <vector>
#include <iterator>
#include <memory>
#include "gtest/gtest.h"
#include "test_common.hpp"

using namespace std;

template <size_t Index = 0>
struct SummedTupleExtractor {

    template <typename... Ts>
    auto operator()(std::shared_ptr<std::tuple<Ts...>> const& t) const {
        return std::get<Index>(*t);
    }
};

template <size_t Index = 0>
struct PointerTupleExtractor {

    template <typename... Ts>
    auto operator()(std::shared_ptr<std::tuple<Ts...>> const& t) const {
        return &std::as_const(std::get<Index>(*t));
    }

    template <typename... Ts>
    [[nodiscard]] auto get_key(std::shared_ptr<std::tuple<Ts...>> const& t) const {
        return operator()(t);
    }

    template <typename... Ts>
    [[nodiscard]] auto const& get_value(std::shared_ptr<std::tuple<Ts...>> const& t) const {
        return t;
    }
};

struct PointerComparator {
    template <typename T, typename U>
    bool operator()(T const* t, U const* u) const {
        return *t < *u;
    }
};

TEST(BppTreeTest, TestReferenceWrapperTransient) {
    uint64_t allocations2 = 0;
    uint64_t deallocations2 = 0;
    {
        static constexpr int n = 1000 * 1000;
        Vector<int32_t> const& rand_ints = RandInts<int32_t, n>::ints;
        typename OrderedTree<std::shared_ptr<std::tuple<int, int>>, PointerTupleExtractor<0>, int64_t, SummedTupleExtractor<1>, PointerComparator>::Transient tree{};
        int start = 0;
        int end = n - 1;
        auto start_time = std::chrono::steady_clock::now();
        for (int i = 0; i < n / 2; i++) {
            tree.insert_or_assign(std::shared_ptr<std::tuple<int, int>>(new std::tuple(end, rand_ints[end]), [&deallocations2](std::tuple<int, int>* t){ delete t; ++deallocations2; }));
            tree.insert_or_assign(std::shared_ptr<std::tuple<int, int>>(new std::tuple(start, rand_ints[start]), [&deallocations2](std::tuple<int, int>* t){ delete t; ++deallocations2; }));
            ++start;
            --end;
            allocations2 += 2;
        }
        cout << "shared_ptr allocations: " << allocations2 << endl;
        cout << "shared_ptr deallocations: " << deallocations2 << endl;
        auto end_time = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsed = end_time - start_time;
        cout << elapsed.count() << 's' << endl;
        EXPECT_TRUE(std::is_sorted(tree.begin(), tree.end(),
                                   [](auto const& a, auto const& b) { return *a.get() < *b.get(); }));
        auto sorted_rand_ints = rand_ints;
        std::sort(sorted_rand_ints.begin(), sorted_rand_ints.end());
        int64_t sum = 0;
        start_time = std::chrono::steady_clock::now();
        for (int i = 0; i < n; i++) {
            sum += std::get<1>(*tree.at_key(&sorted_rand_ints[i]));
            auto it = tree.find(&sorted_rand_ints[i]);
            ASSERT_EQ(sum, tree.sum_inclusive(it));
        }
        end_time = std::chrono::steady_clock::now();
        elapsed = end_time - start_time;
        cout << elapsed.count() << 's' << endl;
        cout << sum << endl;
        cout << tree.sum() << endl;
        cout << "size: " << tree.size() << ", n: " << n << endl;
        cout << "inserting elements again" << endl;
        start = 0;
        end = n - 1;
        for (int i = 0; i < n / 2; i++) {
            tree.insert_or_assign(std::shared_ptr<std::tuple<int, int>>(new std::tuple(end, rand_ints[end]), [&deallocations2](std::tuple<int, int>* t){ delete t; ++deallocations2; }));
            tree.insert_or_assign(std::shared_ptr<std::tuple<int, int>>(new std::tuple(start, rand_ints[start]), [&deallocations2](std::tuple<int, int>* t){ delete t; ++deallocations2; }));
            ++start;
            --end;
            allocations2 += 2;
        }
        cout << "size: " << tree.size() << ", n: " << n << endl;
        cout << "shared_ptr allocations: " << allocations2 << endl;
        cout << "shared_ptr deallocations: " << deallocations2 << endl;
        cout << "deleting every other element from ordered tree" << endl;
        sorted_rand_ints.clear();
        int deleted_keys = 0;
        for (int i = 0; i < n; i++) {
            if (i % 2 == 0) {
                tree.erase_key(&rand_ints[i]);
                ++deleted_keys;
                ASSERT_EQ(deleted_keys, deallocations2 - 1000000);
            } else {
                sorted_rand_ints.push_back(rand_ints[i] % n);
            }
        }
        sort(sorted_rand_ints.begin(), sorted_rand_ints.end());
        cout << "deleted " << deleted_keys << " keys" << endl;
        cout << "size: " << tree.size() << endl;
        cout << "shared_ptr allocations: " << allocations2 << endl;
        cout << "shared_ptr deallocations: " << deallocations2 << endl;
        sum = 0;
        for (size_t i = 0; i < sorted_rand_ints.size(); i++) {
            sum += std::get<1>(*tree.at_key(&sorted_rand_ints[i]));
            auto it = tree.find(&sorted_rand_ints[i]);
            EXPECT_EQ(*it.get(), *tree.at_key(&sorted_rand_ints[i]));
            EXPECT_EQ(sum, tree.sum_inclusive(it));
        }
        EXPECT_EQ(sorted_rand_ints.size(), tree.size());
        deleted_keys = 0;
        cout << "deleting remaining keys" << endl;
        for (int i = 0; i < n; i++) {
            if (i % 2 == 1) {
                tree.erase_key(&rand_ints[i]);
                ++deleted_keys;
                ASSERT_EQ(deleted_keys, deallocations2 - 1500000);
            }
        }
        cout << "deleted " << deleted_keys << " keys" << endl;
        cout << "size: " << tree.size() << endl;
        cout << "shared_ptr allocations: " << allocations2 << endl;
        cout << "shared_ptr deallocations: " << deallocations2 << endl;
        cout << "leaving scope, tree will be deleted" << endl;
    }
    cout << "shared_ptr allocations: " << allocations2 << endl;
    cout << "shared_ptr deallocations: " << deallocations2 << endl;
}

TEST(BppTreeTest, TestReferenceWrapperPersistent) {
    uint64_t allocations2 = 0;
    uint64_t deallocations2 = 0;
    {
        static constexpr int n = 100 * 1000;
        Vector<int32_t> const& rand_ints = RandInts<int32_t, n>::ints;
        using TreeType = OrderedTree<std::shared_ptr<std::tuple<int, int>>, PointerTupleExtractor<0>, int64_t, SummedTupleExtractor<1>, PointerComparator>::Persistent;
        TreeType tree{};
        int start = 0;
        int end = n - 1;
        auto start_time = std::chrono::steady_clock::now();
        for (int i = 0; i < n / 2; i++) {
            tree = tree.insert_or_assign(std::shared_ptr<std::tuple<int, int>>(new std::tuple(end, rand_ints[end]), [&deallocations2](std::tuple<int, int>* t){ delete t; ++deallocations2; }));
            tree = tree.insert_or_assign(std::shared_ptr<std::tuple<int, int>>(new std::tuple(start, rand_ints[start]), [&deallocations2](std::tuple<int, int>* t){ delete t; ++deallocations2; }));
            ++start;
            --end;
            allocations2 += 2;
        }
        cout << "shared_ptr allocations: " << allocations2 << endl;
        cout << "shared_ptr deallocations: " << deallocations2 << endl;
        auto end_time = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsed = end_time - start_time;
        cout << elapsed.count() << 's' << endl;
        EXPECT_TRUE(std::is_sorted(tree.begin(), tree.end(),
                                   [](auto const& a, auto const& b) { return *a.get() < *b.get(); }));
        auto sorted_rand_ints = rand_ints;
        std::sort(sorted_rand_ints.begin(), sorted_rand_ints.end());
        int64_t sum = 0;
        start_time = std::chrono::steady_clock::now();
        for (int i = 0; i < n; i++) {
            sum += std::get<1>(*tree.at_key(&sorted_rand_ints[i]));
            auto it = tree.find(&sorted_rand_ints[i]);
            ASSERT_EQ(sum, tree.sum_inclusive(it));
        }
        end_time = std::chrono::steady_clock::now();
        elapsed = end_time - start_time;
        cout << elapsed.count() << 's' << endl;
        cout << sum << endl;
        cout << tree.sum() << endl;
        cout << "size: " << tree.size() << ", n: " << n << endl;
        cout << "inserting elements again" << endl;
        start = 0;
        end = n - 1;
        for (int i = 0; i < n / 2; i++) {
            tree = tree.insert_or_assign(std::shared_ptr<std::tuple<int, int>>(new std::tuple(end, rand_ints[end]), [&deallocations2](std::tuple<int, int>* t){ delete t; ++deallocations2; }));
            tree = tree.insert_or_assign(std::shared_ptr<std::tuple<int, int>>(new std::tuple(start, rand_ints[start]), [&deallocations2](std::tuple<int, int>* t){ delete t; ++deallocations2; }));
            ++start;
            --end;
            allocations2 += 2;
        }
        cout << "size: " << tree.size() << ", n: " << n << endl;
        cout << "shared_ptr allocations: " << allocations2 << endl;
        cout << "shared_ptr deallocations: " << deallocations2 << endl;
        cout << "deleting every other element from ordered tree" << endl;
        sorted_rand_ints.clear();
        int deleted_keys = 0;
        for (int i = 0; i < n; i++) {
            if (i % 2 == 0) {
                tree = tree.erase_key(&rand_ints[i]);
                ++deleted_keys;
                ASSERT_EQ(deleted_keys, deallocations2 - 100000);
            } else {
                sorted_rand_ints.push_back(rand_ints[i] % n);
            }
        }
        sort(sorted_rand_ints.begin(), sorted_rand_ints.end());
        cout << "deleted " << deleted_keys << " keys" << endl;
        cout << "size: " << tree.size() << endl;
        cout << "shared_ptr allocations: " << allocations2 << endl;
        cout << "shared_ptr deallocations: " << deallocations2 << endl;
        sum = 0;
        for (size_t i = 0; i < sorted_rand_ints.size(); i++) {
            sum += std::get<1>(*tree.at_key(&sorted_rand_ints[i]));
            auto it = tree.find(&sorted_rand_ints[i]);
            EXPECT_EQ(*it.get(), *tree.at_key(&sorted_rand_ints[i]));
            EXPECT_EQ(sum, tree.sum_inclusive(it));
        }
        EXPECT_EQ(sorted_rand_ints.size(), tree.size());
        deleted_keys = 0;
        cout << "deleting remaining keys" << endl;
        for (int i = 0; i < n; i++) {
            if (i % 2 == 1) {
               tree = tree.erase_key(&rand_ints[i]);
                ++deleted_keys;
                ASSERT_EQ(deleted_keys, deallocations2 - 150000);
            }
        }
        cout << "deleted " << deleted_keys << " keys" << endl;
        cout << "size: " << tree.size() << endl;
        cout << "shared_ptr allocations: " << allocations2 << endl;
        cout << "shared_ptr deallocations: " << deallocations2 << endl;
        cout << "leaving scope, tree will be deleted" << endl;
    }
    cout << "shared_ptr allocations: " << allocations2 << endl;
    cout << "shared_ptr deallocations: " << deallocations2 << endl;
}

TEST(BppTreeTest, TestRefWrapMin) {
    using TreeType = BppTree<std::shared_ptr<tuple<uint32_t, uint32_t>>, 256, 256, 6>
            ::mixins<
                    IndexedBuilder<>,
                    MinBuilder<>::extractor<PointerTupleExtractor<0>>::compare<PointerComparator>>;
    TreeType::Transient tree{};
    Vector<uint32_t> vec{};
    for (uint32_t i = 0; i < 1024; ++i) {
        auto r = static_cast<uint32_t>(rand());
        tree.emplace_back(std::make_shared<std::tuple<uint32_t, uint32_t>>(r, i));
        vec.emplace_back(r);
    }
    for (uint32_t i = 0; i < tree.size(); ++i) {
        tuple<uint32_t, uint32_t> t = *tree[i].get();
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
            EXPECT_FALSE(*min != std::get<0>(*tree_it.get()) || *min != *vec_it);
        }
    }
    auto vec_it = std::min_element(vec.begin(), vec.end());
    auto min = tree.min();
    auto tree_it = tree.min_element();
    EXPECT_FALSE(*min != std::get<0>(*tree_it.get()) || *min != *vec_it);
    std::sort(tree.begin(), tree.end(), [](auto const& a, auto const& b){ return *a.get() < *b.get(); });
    EXPECT_TRUE(std::is_sorted(tree.begin(), tree.end(), [](auto const& a, auto const& b){ return *a.get() < *b.get(); }));
    for (uint32_t i = 0; i < tree.size(); ++i) {
        ASSERT_EQ(std::get<0>(**tree.min_element_const(tree.begin()+i, tree.end())), *tree.min(tree.begin()+i, tree.end()));
        ASSERT_EQ(**tree.min_element_const(tree.begin()+i, tree.end()), *tree[i].get());
    }
}
