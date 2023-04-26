#include "bpptree/bpptree.hpp"
#include <iostream>
#include <chrono>
#include <vector>
#include <iterator>
#include "gtest/gtest.h"
#include "test_common.hpp"

using namespace std;

template <bool enable_persistence, typename T, typename SizeType>
static void run_test(size_t n, std::vector<T> const& rand_ints) {
    {
        using TreeType = std::conditional_t<
                enable_persistence,
                typename SummedIndexedTree<T, SizeType>::Persistent,
                typename SummedIndexedTree<T, SizeType>::Transient>;
        TreeType tree{};
        auto start_time = std::chrono::steady_clock::now();
        for (size_t i = 0; i < n / 2; i++) {
            if constexpr (enable_persistence) {
                tree = tree.insert_index(i * 2, rand_ints[i * 2]);
                tree = tree.insert_index(i * 2 + 1, rand_ints[i * 2 + 1]);
            } else {
                for (size_t j = 0; j < 2; j++) {
                    EXPECT_EQ(tree.size(), i * 2 + j);
                    tree.emplace_back(rand_ints[i * 2 + j]);
                    EXPECT_EQ(tree[tree.size() - 1].get(), rand_ints[i * 2 + j]);
                }
            }
        }
        auto end_time = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsed = end_time - start_time;
        cout << elapsed.count() << 's' << endl;

        cout << "allocations: " << allocations << " deallocations : " << deallocations << endl;
        cout << "increments: " << increments << " decrements : " << decrements << endl;

        cout << "size: " << tree.size() << endl;

        EXPECT_EQ(tree.size(), n);

        for (int j = 0; j < 5; j++) {
            uint64_t sum = 0;
            start_time = std::chrono::steady_clock::now();
            for (size_t i = 0; i < n; i++) {
                sum += static_cast<uint64_t const>(tree[i]);
            }
            end_time = std::chrono::steady_clock::now();
            elapsed = end_time - start_time;
            if (j == 0) {
                EXPECT_EQ(sum, tree.sum());
                cout << sum << endl;
                cout << tree.sum() << endl;
            }
            cout << elapsed.count() << 's' << endl;
        }
        for (int j = 0; j < 5; j++) {
            uint64_t sum = 0;
            start_time = std::chrono::steady_clock::now();
            for (size_t i = 0; i < n; i++) {
                sum += static_cast<uint64_t const>(std::as_const(tree)[i]);
            }
            end_time = std::chrono::steady_clock::now();
            elapsed = end_time - start_time;
            if (j == 0) {
                EXPECT_EQ(sum, tree.sum());
                cout << sum << endl;
                cout << tree.sum() << endl;
            }
            cout << elapsed.count() << 's' << endl;
        }
        for (int j = 0; j < 10; j++) {
            uint64_t sum = 0;
            start_time = std::chrono::steady_clock::now();
            for (T t : tree) {
                sum += unsigned_cast(t);
            }
            end_time = std::chrono::steady_clock::now();
            elapsed = end_time - start_time;
            if (j == 0) {
                EXPECT_EQ(sum, tree.sum());
                cout << sum << endl;
                cout << tree.sum() << endl;
            }
            cout << elapsed.count() << 's' << endl;
        }
        for (int j = 0; j < 10; j++) {
            uint64_t sum = 0;
            start_time = std::chrono::steady_clock::now();
            for (T t : rand_ints) {
                sum += unsigned_cast(t);
            }
            end_time = std::chrono::steady_clock::now();
            elapsed = end_time - start_time;
            if (j == 0) {
                EXPECT_EQ(sum, tree.sum());
                cout << sum << endl;
                cout << tree.sum() << endl;
            }
            cout << elapsed.count() << 's' << endl;
        }
        for (int j = 0; j < 1; j++) {
            uint64_t sum = 0;
            start_time = std::chrono::steady_clock::now();
            for (size_t i = 0; i < n; i++) {
                sum += static_cast<uint64_t>(*tree.find_index(i));
            }
            end_time = std::chrono::steady_clock::now();
            elapsed = end_time - start_time;
            if (j == 0) {
                cout << sum << endl;
                cout << tree.sum() << endl;
            }
            cout << "get_iterator time: " << elapsed.count() << 's' << endl;
        }
        for (int j = 0; j < 10; ++j) {
            SizeType sum = 0;
            start_time = std::chrono::steady_clock::now();
            auto begin = tree.begin();
            if (j == 0) {
                cout << "sizeof(iterator): " << sizeof(decltype(begin)) << endl;
                cout << "sizeof(begin): " << sizeof(begin) << endl;
            }
            auto end2 = tree.end();
            for (; begin != end2; ++begin) {
                sum += *begin;
                EXPECT_EQ(sum, tree.sum_inclusive(begin));
                auto ceil_it = tree.sum_lower_bound(sum);
                EXPECT_FALSE(*begin > 0 && ceil_it != begin);
            }
            end_time = std::chrono::steady_clock::now();
            elapsed = end_time - start_time;
            cout << sum << endl;
            cout << elapsed.count() << 's' << endl;
            if (j == 0) {
                cout << "subtracting 1 from all values" << endl;
                start_time = std::chrono::steady_clock::now();
                if constexpr (enable_persistence) {
                    for (size_t i = 0; i < n; ++i) {
                        auto val = tree[i];
                        tree = tree.assign_index(i, val == 0 ? 0 : (val - 1));
                    }
                } else {
                    auto begin2 = tree.begin();
                    auto end3 = tree.end();
                    for (; begin2 != end3; ++begin2) {
                        *begin2 = *begin2 == 0 ? 0 : *begin2 - 1;
                    }
                }
                end_time = std::chrono::steady_clock::now();
                elapsed = end_time - start_time;
                cout << tree.sum() << endl;
                cout << elapsed.count() << 's' << endl;
            }
            if (j == 1) {
                cout << "adding 1 to all values" << endl;
                start_time = std::chrono::steady_clock::now();
                if constexpr (enable_persistence) {
                    for (size_t i = 0; i < n; ++i) {
                        tree = tree.assign_index(i, tree[i] + 1);
                    }
                } else {
                    auto begin2 = tree.begin();
                    auto end3 = tree.end();
                    for (; begin2 != end3; ++begin2) {
                        *begin2 += static_cast<T>(1);
                    }
                }
                end_time = std::chrono::steady_clock::now();
                elapsed = end_time - start_time;
                cout << tree.sum() << endl;
                cout << elapsed.count() << 's' << endl;
            }
            if (j == 2) {
                cout << "subtracting 1 from all values again" << endl;
                start_time = std::chrono::steady_clock::now();
                if constexpr (enable_persistence) {
                    for (size_t i = 0; i < n; ++i) {
                        tree = tree.assign_index(i, tree[i] - 1);
                    }
                } else {
                    for (size_t i = 0; i < n; ++i) {
                        tree[i] -= static_cast<T>(1);
                    }
                }
                end_time = std::chrono::steady_clock::now();
                elapsed = end_time - start_time;
                cout << tree.sum() << endl;
                cout << elapsed.count() << 's' << endl;
            }
            if (j == 3) {
                cout << "adding 1 to all values again" << endl;
                start_time = std::chrono::steady_clock::now();
                if constexpr (enable_persistence) {
                    for (size_t i = 0; i < n; ++i) {
                        tree = tree.assign_index(i, tree[i] + 1);
                    }
                } else {
                    for (size_t i = 0; i < n; ++i) {
                        tree[i] = tree[i] + 1;
                    }
                }
                end_time = std::chrono::steady_clock::now();
                elapsed = end_time - start_time;
                cout << tree.sum() << endl;
                cout << elapsed.count() << 's' << endl;
            }
            if (j == 4) {
                cout << "subtracting 1 from all values" << endl;
                start_time = std::chrono::steady_clock::now();
                if constexpr (enable_persistence) {
                    for (size_t i = 0; i < n; ++i) {
                        tree = tree.assign_index(i, tree[i] - 1);
                    }
                } else {
                    auto begin2 = tree.begin();
                    auto end3 = tree.end();
                    for (; begin2 != end3; ++begin2) {
                        --*begin2;
                    }
                }
                end_time = std::chrono::steady_clock::now();
                elapsed = end_time - start_time;
                cout << tree.sum() << endl;
                cout << elapsed.count() << 's' << endl;
            }
            if (j == 5) {
                cout << "adding 1 to all values" << endl;
                start_time = std::chrono::steady_clock::now();
                if constexpr (enable_persistence) {
                    for (size_t i = 0; i < n; ++i) {
                        tree = tree.assign_index(i, tree[i] + 1);
                    }
                } else {
                    auto begin2 = tree.begin();
                    auto end3 = tree.end();
                    for (; begin2 != end3; ++begin2) {
                        (*begin2)++;
                    }
                }
                end_time = std::chrono::steady_clock::now();
                elapsed = end_time - start_time;
                cout << tree.sum() << endl;
                cout << elapsed.count() << 's' << endl;
            }
            if (j == 6) {
                cout << "subtracting 1 from all values again" << endl;
                start_time = std::chrono::steady_clock::now();
                if constexpr (enable_persistence) {
                    for (size_t i = 0; i < n; ++i) {
                        tree = tree.assign_index(i, tree[i] - 1);
                    }
                } else {
                    for (size_t i = 0; i < n; ++i) {
                        tree[i]--;
                    }
                }
                end_time = std::chrono::steady_clock::now();
                elapsed = end_time - start_time;
                cout << tree.sum() << endl;
                cout << elapsed.count() << 's' << endl;
            }
            if (j == 7) {
                cout << "adding 1 to all values again" << endl;
                start_time = std::chrono::steady_clock::now();
                if constexpr (enable_persistence) {
                    for (size_t i = 0; i < n; ++i) {
                        tree = tree.assign_index(i, tree[i] + 1);
                    }
                } else {
                    for (size_t i = 0; i < n; ++i) {
                        ++tree[i];
                    }
                }
                end_time = std::chrono::steady_clock::now();
                elapsed = end_time - start_time;
                cout << tree.sum() << endl;
                cout << elapsed.count() << 's' << endl;
            }
            if (j == 8) {
                cout << "erasing every other value" << endl;
                start_time = std::chrono::steady_clock::now();
                for (size_t i = 0; i < n/2; ++i) {
                    if constexpr (enable_persistence) {
                        tree = tree.erase_index(i);
                    } else {
                        tree.erase_index(i);
                    }
                }
                end_time = std::chrono::steady_clock::now();
                elapsed = end_time - start_time;
                cout << tree.sum() << endl;
                cout << elapsed.count() << 's' << endl;
                cout << "size: " << tree.size() << endl;
            }
        }
        if constexpr (!enable_persistence) {
            sort(tree.begin(), tree.end());
            EXPECT_TRUE(is_sorted(tree.begin(), tree.end()));
            tree.clear();
            EXPECT_TRUE(tree.empty());
        }
    }
    cout << "allocations: " << allocations << " deallocations : " << deallocations << endl;
    cout << "increments: " << increments << " decrements : " << decrements << endl;
    reset_counters();
}

TEST(BppTreeTest, TestSignedTransient) {
    run_test<false, int32_t, int64_t>(num_ints_large, RandInts<int32_t, num_ints_large>::ints);
}

TEST(BppTreeTest, TestSignedPersistent) {
    run_test<true, int32_t, int64_t>(num_ints_small, RandInts<int32_t, num_ints_small>::ints);
}

TEST(BppTreeTest, TestUnsignedTransient) {
    run_test<false, uint32_t, uint64_t>(num_ints_large, RandInts<uint32_t, num_ints_large>::ints);
}

TEST(BppTreeTest, TestUnsignedPersistent) {
    run_test<true, uint32_t, uint64_t>(num_ints_small, RandInts<uint32_t, num_ints_small>::ints);
}
