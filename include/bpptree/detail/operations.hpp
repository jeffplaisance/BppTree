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
    void operator()(N& node, T const &search_val, F const& finder, R&& do_replace, S&&, E&&, size_t&, uint64_t& iter, bool, Args&&... args) {
        node.assign(search_val, finder, do_replace, iter, std::forward<decltype(args)>(args)...);
    }
};

struct Erase {
    template<typename N, typename T, typename F, typename R, typename S, typename E>
    void operator()(N& node, T const &search_val, F const& finder, R&& do_replace, S&&, E&& do_erase, size_t& size, uint64_t& iter, bool right_most) {
        node.erase(search_val, finder, do_replace, do_erase, size, iter, right_most);
    }
};

struct Insert {
    template<typename N, typename T, typename F, typename R, typename S, typename E, typename... Args>
    void operator()(N& node, T const &search_val, F const& finder, R&& do_replace, S&& do_split, E&&, size_t &size, uint64_t& iter, bool right_most, Args&&... args) {
        node.insert(search_val, finder, do_replace, do_split, size, iter, right_most, std::forward<decltype(args)>(args)...);
    }
};

struct Update {
    template<typename N, typename T, typename F, typename R, typename S, typename E, typename U>
    void operator()(N& node, T const &search_val, F const& finder, R&& do_replace, S&&, E&&, size_t&, uint64_t& iter, bool, U&& updater) {
        node.update(search_val, finder, do_replace, iter, updater);
    }
};

struct Update2 {
    template<typename N, typename T, typename F, typename R, typename S, typename E, typename U>
    void operator()(N& node, T const &search_val, F const& finder, R&& do_replace, S&&, E&&, size_t&, uint64_t& iter, bool, U&& updater) {
        node.update2(search_val, finder, do_replace, iter, updater);
    }
};

template <DuplicatePolicy duplicate_policy>
struct InsertOrAssign {
    template<typename N, typename T, typename F, typename R, typename S, typename E, typename... Args>
    void operator()(N& node, T const &search_val, F const& finder, R&& do_replace, S&& do_split, E&&,
            size_t &size, uint64_t& iter, bool right_most, Args&&... args) {
        node.template insert_or_assign<duplicate_policy>(search_val, finder, do_replace, do_split,
                            size, iter, right_most, std::forward<decltype(args)>(args)...);
    }
};

static constexpr auto find_first = [](auto const&, Empty const&) {
    return std::tuple<int32_t, Empty>(0, Empty::empty);
};

static constexpr auto find_last = [](auto const& node, Empty const&) {
    if constexpr (std::remove_reference_t<decltype(node)>::depth == 1) {
        return std::tuple<int32_t, Empty>(node.length, Empty::empty);
    } else {
        return std::tuple<int32_t, Empty>(node.length - 1, Empty::empty);
    }
};

static constexpr auto find_iterator = [](auto const& node, uint64_t const& it) {
    return std::tuple<int32_t, uint64_t>(node.get_index(it), it);
};
} //end namespace bpptree::detail
