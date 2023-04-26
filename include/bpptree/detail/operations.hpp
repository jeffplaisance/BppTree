//
// B++ Tree: A B+ Tree library written in C++
// Copyright (C) 2023 Jeff Plaisance
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#pragma once

#include <cstdint>
#include <utility>
#include "helpers.hpp"

namespace bpptree::detail {

struct Assign {
    template<typename N, typename T, typename F, typename R, typename S, typename E, typename... Args>
    void operator()(N& node, T const &searchVal, F&& finder, R&& doReplace, S&&, E&&, size_t&, uint64_t& iter, bool, Args&&... args) {
        node.assign(searchVal, finder, doReplace, iter, std::forward<decltype(args)>(args)...);
    }
};

struct Erase {
    template<typename N, typename T, typename F, typename R, typename S, typename E>
    void operator()(N& node, T const &searchVal, F&& finder, R&& doReplace, S&&, E&& doErase, size_t& size, uint64_t& iter, bool rightMost) {
        node.erase(searchVal, finder, doReplace, doErase, size, iter, rightMost);
    }
};

struct Insert {
    template<typename N, typename T, typename F, typename R, typename S, typename E, typename... Args>
    void operator()(N& node, T const &searchVal, F&& finder, R&& doReplace, S&& doSplit, E&&, size_t &size, uint64_t& iter, bool rightMost, Args&&... args) {
        node.insert(searchVal, finder, doReplace, doSplit, size, iter, rightMost, std::forward<decltype(args)>(args)...);
    }
};

struct Update {
    template<typename N, typename T, typename F, typename R, typename S, typename E, typename U>
    void operator()(N& node, T const &searchVal, F&& finder, R&& doReplace, S&&, E&&, size_t&, uint64_t& iter, bool, U&& updater) {
        node.update(searchVal, finder, doReplace, iter, updater);
    }
};

struct Update2 {
    template<typename N, typename T, typename F, typename R, typename S, typename E, typename U>
    void operator()(N& node, T const &searchVal, F&& finder, R&& doReplace, S&&, E&&, size_t&, uint64_t& iter, bool, U&& updater) {
        node.update2(searchVal, finder, doReplace, iter, updater);
    }
};

template <DuplicatePolicy duplicate_policy>
struct InsertOrAssign {
    template<typename N, typename T, typename F, typename R, typename S, typename E, typename... Args>
    void operator()(N& node, T const &searchVal, F&& finder, R&& doReplace, S&& doSplit, E&&,
            size_t &size, uint64_t& iter, bool rightMost, Args&&... args) {
        node.template insertOrAssign<duplicate_policy>(searchVal, finder, doReplace, doSplit,
                            size, iter, rightMost, std::forward<decltype(args)>(args)...);
    }
};

struct FindFirst {
    template <typename Node>
    auto operator()(Node const&, Empty const&) {
        return std::tuple<int32_t, Empty>(0, Empty::empty);
    }
};

struct FindLast {
    template <typename Node>
    auto operator()(Node const& node, Empty const&) {
        if constexpr (Node::depth == 1) {
            return std::tuple<int32_t, Empty>(node.length, Empty::empty);
        } else {
            return std::tuple<int32_t, Empty>(node.length - 1, Empty::empty);
        }
    }
};

struct FindIterator {
    template <typename Node>
    auto operator()(Node const& node, uint64_t const& it) {
        return std::tuple<int32_t, uint64_t>(node.getIndex(it), it);
    }
};
} //end namespace bpptree::detail
