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
    uint64_t allocations = 0;
    uint64_t deallocations = 0;
    {
        static constexpr int n = 1000 * 1000;
        safe_vector<int32_t> const& rand_ints = RandInts<int32_t, n>::ints;
        typename OrderedTree<std::shared_ptr<std::tuple<int, int>>, PointerTupleExtractor<0>, int64_t, SummedTupleExtractor<1>, PointerComparator>::Transient tree{};
        int start = 0;
        int end = n - 1;
        auto startTime = std::chrono::steady_clock::now();
        for (int i = 0; i < n / 2; i++) {
            tree.insert_or_assign(std::shared_ptr<std::tuple<int, int>>(new std::tuple(end, rand_ints[end]), [&deallocations](std::tuple<int, int>* t){ delete t; ++deallocations; }));
            tree.insert_or_assign(std::shared_ptr<std::tuple<int, int>>(new std::tuple(start, rand_ints[start]), [&deallocations](std::tuple<int, int>* t){ delete t; ++deallocations; }));
            ++start;
            --end;
            allocations += 2;
        }
        cout << "shared_ptr allocations: " << allocations << endl;
        cout << "shared_ptr deallocations: " << deallocations << endl;
        auto endTime = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsed = endTime - startTime;
        cout << elapsed.count() << 's' << endl;
        EXPECT_TRUE(std::is_sorted(tree.begin(), tree.end(),
                                   [](auto const& a, auto const& b) { return *a.get() < *b.get(); }));
        auto sorted_rand_ints = rand_ints;
        std::sort(sorted_rand_ints.begin(), sorted_rand_ints.end());
        int64_t sum = 0;
        startTime = std::chrono::steady_clock::now();
        for (int i = 0; i < n; i++) {
            sum += std::get<1>(*tree.at_key(&sorted_rand_ints[i]));
            auto it = tree.find(&sorted_rand_ints[i]);
            ASSERT_EQ(sum, tree.sum_inclusive(it));
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
            tree.insert_or_assign(std::shared_ptr<std::tuple<int, int>>(new std::tuple(end, rand_ints[end]), [&deallocations](std::tuple<int, int>* t){ delete t; ++deallocations; }));
            tree.insert_or_assign(std::shared_ptr<std::tuple<int, int>>(new std::tuple(start, rand_ints[start]), [&deallocations](std::tuple<int, int>* t){ delete t; ++deallocations; }));
            ++start;
            --end;
            allocations += 2;
        }
        cout << "size: " << tree.size() << ", n: " << n << endl;
        cout << "shared_ptr allocations: " << allocations << endl;
        cout << "shared_ptr deallocations: " << deallocations << endl;
        cout << "deleting every other element from ordered tree" << endl;
        sorted_rand_ints.clear();
        int deleted_keys = 0;
        for (int i = 0; i < n; i++) {
            if (i % 2 == 0) {
                tree.erase_key(&rand_ints[i]);
                ++deleted_keys;
                ASSERT_EQ(deleted_keys, deallocations - 1000000);
            } else {
                sorted_rand_ints.push_back(rand_ints[i] % n);
            }
        }
        sort(sorted_rand_ints.begin(), sorted_rand_ints.end());
        cout << "deleted " << deleted_keys << " keys" << endl;
        cout << "size: " << tree.size() << endl;
        cout << "shared_ptr allocations: " << allocations << endl;
        cout << "shared_ptr deallocations: " << deallocations << endl;
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
                ASSERT_EQ(deleted_keys, deallocations - 1500000);
            }
        }
        cout << "deleted " << deleted_keys << " keys" << endl;
        cout << "size: " << tree.size() << endl;
        cout << "shared_ptr allocations: " << allocations << endl;
        cout << "shared_ptr deallocations: " << deallocations << endl;
        cout << "leaving scope, tree will be deleted" << endl;
    }
    cout << "shared_ptr allocations: " << allocations << endl;
    cout << "shared_ptr deallocations: " << deallocations << endl;
}

TEST(BppTreeTest, TestReferenceWrapperPersistent) {
    uint64_t allocations = 0;
    uint64_t deallocations = 0;
    {
        static constexpr int n = 100 * 1000;
        safe_vector<int32_t> const& rand_ints = RandInts<int32_t, n>::ints;
        using TreeType = OrderedTree<std::shared_ptr<std::tuple<int, int>>, PointerTupleExtractor<0>, int64_t, SummedTupleExtractor<1>, PointerComparator>::Persistent;
        TreeType tree{};
        int start = 0;
        int end = n - 1;
        auto startTime = std::chrono::steady_clock::now();
        for (int i = 0; i < n / 2; i++) {
            tree = tree.insert_or_assign(std::shared_ptr<std::tuple<int, int>>(new std::tuple(end, rand_ints[end]), [&deallocations](std::tuple<int, int>* t){ delete t; ++deallocations; }));
            tree = tree.insert_or_assign(std::shared_ptr<std::tuple<int, int>>(new std::tuple(start, rand_ints[start]), [&deallocations](std::tuple<int, int>* t){ delete t; ++deallocations; }));
            ++start;
            --end;
            allocations += 2;
        }
        cout << "shared_ptr allocations: " << allocations << endl;
        cout << "shared_ptr deallocations: " << deallocations << endl;
        auto endTime = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsed = endTime - startTime;
        cout << elapsed.count() << 's' << endl;
        EXPECT_TRUE(std::is_sorted(tree.begin(), tree.end(),
                                   [](auto const& a, auto const& b) { return *a.get() < *b.get(); }));
        auto sorted_rand_ints = rand_ints;
        std::sort(sorted_rand_ints.begin(), sorted_rand_ints.end());
        int64_t sum = 0;
        startTime = std::chrono::steady_clock::now();
        for (int i = 0; i < n; i++) {
            sum += std::get<1>(*tree.at_key(&sorted_rand_ints[i]));
            auto it = tree.find(&sorted_rand_ints[i]);
            ASSERT_EQ(sum, tree.sum_inclusive(it));
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
            tree = tree.insert_or_assign(std::shared_ptr<std::tuple<int, int>>(new std::tuple(end, rand_ints[end]), [&deallocations](std::tuple<int, int>* t){ delete t; ++deallocations; }));
            tree = tree.insert_or_assign(std::shared_ptr<std::tuple<int, int>>(new std::tuple(start, rand_ints[start]), [&deallocations](std::tuple<int, int>* t){ delete t; ++deallocations; }));
            ++start;
            --end;
            allocations += 2;
        }
        cout << "size: " << tree.size() << ", n: " << n << endl;
        cout << "shared_ptr allocations: " << allocations << endl;
        cout << "shared_ptr deallocations: " << deallocations << endl;
        cout << "deleting every other element from ordered tree" << endl;
        sorted_rand_ints.clear();
        int deleted_keys = 0;
        for (int i = 0; i < n; i++) {
            if (i % 2 == 0) {
                tree = tree.erase_key(&rand_ints[i]);
                ++deleted_keys;
                ASSERT_EQ(deleted_keys, deallocations - 100000);
            } else {
                sorted_rand_ints.push_back(rand_ints[i] % n);
            }
        }
        sort(sorted_rand_ints.begin(), sorted_rand_ints.end());
        cout << "deleted " << deleted_keys << " keys" << endl;
        cout << "size: " << tree.size() << endl;
        cout << "shared_ptr allocations: " << allocations << endl;
        cout << "shared_ptr deallocations: " << deallocations << endl;
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
                ASSERT_EQ(deleted_keys, deallocations - 150000);
            }
        }
        cout << "deleted " << deleted_keys << " keys" << endl;
        cout << "size: " << tree.size() << endl;
        cout << "shared_ptr allocations: " << allocations << endl;
        cout << "shared_ptr deallocations: " << deallocations << endl;
        cout << "leaving scope, tree will be deleted" << endl;
    }
    cout << "shared_ptr allocations: " << allocations << endl;
    cout << "shared_ptr deallocations: " << deallocations << endl;
}

TEST(BppTreeTest, TestRefWrapMin) {
    using TreeType = BppTree<std::shared_ptr<tuple<uint32_t, uint32_t>>, 256, 256, 6>
            ::mixins<
                    IndexedBuilder<>,
                    MinBuilder<>::extractor<PointerTupleExtractor<0>>::compare<PointerComparator>>;
    TreeType::Transient tree{};
    safe_vector<uint32_t> vec{};
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
            const auto treeBegin = tree.begin() + i;
            const auto treeEnd = tree.begin() + j;
            EXPECT_LT(treeBegin, treeEnd);
            EXPECT_FALSE(treeEnd < treeBegin);
            EXPECT_LE(treeBegin, treeEnd);
            EXPECT_FALSE(treeEnd <= treeBegin);
            EXPECT_GT(treeEnd, treeBegin);
            EXPECT_FALSE(treeBegin > treeEnd);
            EXPECT_GE(treeEnd, treeBegin);
            EXPECT_FALSE(treeBegin >= treeEnd);
            EXPECT_EQ(treeBegin+(j-i), treeEnd);
            EXPECT_FALSE(treeBegin+(j-i) != treeEnd);
            EXPECT_LE(treeBegin+(j-i), treeEnd);
            EXPECT_LE(treeEnd, treeBegin+(j-i));
            EXPECT_GE(treeBegin+(j-i), treeEnd);
            EXPECT_GE(treeEnd, treeBegin+(j-i));
            EXPECT_FALSE(treeBegin+(j-i) < treeEnd);
            EXPECT_FALSE(treeEnd < treeBegin+(j-i));
            EXPECT_FALSE(treeBegin+(j-i) > treeEnd);
            EXPECT_FALSE(treeEnd > treeBegin+(j-i));
            EXPECT_EQ(treeEnd - treeBegin, j - i);
            auto min = tree.min(treeBegin, treeEnd);
            auto tree_it = tree.min_element(treeBegin, treeEnd);
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
