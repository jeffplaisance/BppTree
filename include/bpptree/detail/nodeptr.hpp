//
// B++ Tree: A B+ Tree library written in C++
// Copyright (C) 2023 Jeff Plaisance
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#pragma once

#include <memory>

namespace bpptree::detail {

inline int allocations = 0;
inline int deallocations = 0;
inline int increments = 0;
inline int decrements = 0;

inline void reset_counters() {
    allocations = 0;
    deallocations = 0;
    increments = 0;
    decrements = 0;
}

static constexpr bool count_allocations =
#ifdef BPPTREE_TEST_COUNT_ALLOCATIONS
    true;
#else
    false;
#endif

template <typename PtrType>
class NodePtr {

    PtrType* ptr = nullptr;

    void dec_ref() {
        if (ptr != nullptr) {
            if (--ptr->ref_count == 0) {
                delete ptr;
                if constexpr (count_allocations) ++deallocations;
            }
            if constexpr (count_allocations) ++decrements;
        }
    }

    void inc_ref() const {
        if (ptr != nullptr) {
            ++ptr->ref_count;
            if constexpr (count_allocations) ++increments;
        }
    }

public:
    using Type = PtrType;

    NodePtr() = default;

    NodePtr(PtrType* p) noexcept : ptr(p) {} //NOLINT

    NodePtr(NodePtr const& other) noexcept : ptr(other.ptr) {
        inc_ref();
    }

    NodePtr(NodePtr&& other) noexcept : ptr(other.ptr) {
        other.ptr = nullptr;
    }

    // NOLINTNEXTLINE(bugprone-unhandled-self-assignment)
    NodePtr& operator=(NodePtr const& rhs) noexcept {
        // call inc_ref on rhs before calling dec_ref on this to ensure safe self-assignment
        rhs.inc_ref();
        dec_ref();
        ptr = rhs.ptr;
        return *this;
    }

    NodePtr& operator=(NodePtr&& rhs) noexcept {
        dec_ref();
        ptr = rhs.ptr;
        rhs.ptr = nullptr;
        return *this;
    }

    PtrType& operator*() {
        return *ptr;
    }

    PtrType const& operator*() const {
        return *ptr;
    }

    PtrType* operator->() {
        return ptr;
    }

    PtrType const* operator->() const {
        return ptr;
    }

public:
    ~NodePtr() {
        dec_ref();
    }
};

template <typename PtrType, typename... Ts>
NodePtr<PtrType> make_ptr(Ts&&... ts) noexcept(std::is_nothrow_constructible_v<PtrType, Ts...>) {
    if constexpr (count_allocations) {
        ++allocations;
        ++increments;
    }
    return NodePtr<PtrType>(new PtrType(std::forward<Ts>(ts)...));
}
} //end namespace bpptree::detail
