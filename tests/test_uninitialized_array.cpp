#include "bpptree/bpptree.hpp"
#include "bpptree/indexed.hpp"
#include <iostream>
#include <chrono>
#include <iterator>
#include "gtest/gtest.h"

struct DestructorChecker {

    size_t unlikely_value;
    bool wasDestructed;

    explicit DestructorChecker(size_t u) {
        EXPECT_EQ(u, 0xdecafc0ffeeadded);
        EXPECT_NE(unlikely_value, u);
        unlikely_value = u;
        wasDestructed = false;
    }

    ~DestructorChecker() {
        EXPECT_FALSE(wasDestructed);
        unlikely_value = 0;
        wasDestructed = true;
    }
};

using namespace std;
using namespace bpptree::detail;

TEST(BppTreeTest, TestUninitializedArray) {
    reset_counters();
    using TreeType = BppTree<DestructorChecker, 128, 128, 10>::mixins2<Indexed>;
    TreeType::Transient tree{};
    tree.emplace_back(0xdecafc0ffeeadded);
    for (uint32_t i = 1; i < 65536; ++i) {
        tree.insert_index(static_cast<uint32_t>(rand()) % tree.size(), 0xdecafc0ffeeadded);
    }
    cout << "allocations: " << allocations << " deallocations : " << deallocations << endl;
    cout << "increments: " << increments << " decrements : " << decrements << endl;
    cout << tree.depth() << endl;
    while (tree.size() > 16384) {
        tree.erase_index(static_cast<uint32_t>(rand()) % tree.size());
    }
    cout << "allocations: " << allocations << " deallocations : " << deallocations << endl;
    cout << "increments: " << increments << " decrements : " << decrements << endl;
    cout << tree.depth() << endl;
    for (uint32_t i = 0; i < 131072; ++i) {
        tree.insert_index(static_cast<uint32_t>(rand()) % tree.size(), 0xdecafc0ffeeadded);
    }
    cout << "allocations: " << allocations << " deallocations : " << deallocations << endl;
    cout << "increments: " << increments << " decrements : " << decrements << endl;
    cout << tree.depth() << endl;
    while (tree.size() > 0) {
        tree.erase_index(static_cast<uint32_t>(rand()) % tree.size());
    }
    cout << "allocations: " << allocations << " deallocations : " << deallocations << endl;
    cout << "increments: " << increments << " decrements : " << decrements << endl;
    cout << tree.depth() << endl;
    reset_counters();
    while (tree.size() < 65536) {
        tree.emplace_back(0xdecafc0ffeeadded);
    }
    cout << "allocations: " << allocations << " deallocations : " << deallocations << endl;
    cout << "increments: " << increments << " decrements : " << decrements << endl;
    cout << tree.depth() << endl;
    while (tree.size() > 127) {
        tree.pop_front();
    }
    cout << "allocations: " << allocations << " deallocations : " << deallocations << endl;
    cout << "increments: " << increments << " decrements : " << decrements << endl;
    cout << tree.depth() << endl;
    while (tree.size() < 131072) {
        tree.emplace_back(0xdecafc0ffeeadded);
    }
    cout << "allocations: " << allocations << " deallocations : " << deallocations << endl;
    cout << "increments: " << increments << " decrements : " << decrements << endl;
    cout << tree.depth() << endl;
    while (tree.size() > 0) {
        tree.pop_front();
        if (tree.size() > 0) {
            tree.pop_back();
        }
    }
    cout << "allocations: " << allocations << " deallocations : " << deallocations << endl;
    cout << "increments: " << increments << " decrements : " << decrements << endl;
    cout << tree.depth() << endl;
}