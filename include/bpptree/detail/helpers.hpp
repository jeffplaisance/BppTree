//
// B++ Tree: A B+ Tree library written in C++
// Copyright (C) 2023 Jeff Plaisance
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#pragma once

#include <tuple>
#include <functional>
#include "nodeptr.hpp"

namespace bpptree {
namespace detail {

using IndexType = std::int_fast32_t;

//a std::array that doesn't cause warnings when you index into it with signed integers when -Wsign-conversion is enabled
template <typename T, size_t n>
struct Array : std::array<T, n> {
    template <typename I, std::enable_if_t<std::is_integral_v<I>, bool> = true>
    decltype(auto) operator[](I const index) {
        return std::array<T, n>::operator[](static_cast<size_t const>(index));
    }

    template <typename I, std::enable_if_t<std::is_integral_v<I>, bool> = true>
    decltype(auto) operator[](I const index) const {
        return std::array<T, n>::operator[](static_cast<size_t const>(index));
    }
};

template <size_t index = 0>
struct TupleExtractor {
    template <typename... Ts>
    auto const& operator()(std::tuple<Ts...> const& t) const {
        return std::get<index>(t);
    }

    template <typename... Ts>
    auto const& operator()(Ts const&... ts) const {
        return std::get<index>(std::forward_as_tuple(ts...));
    }

    template <typename... Ts>
    auto const& get_key(std::tuple<Ts...> const& t) const {
        return std::get<index>(t);
    }

    template <typename... Ts>
    auto const& get_key(Ts const&... ts) const {
        return std::get<index>(std::forward_as_tuple(ts...));
    }

    template <typename F, typename Key, typename V>
    decltype(auto) apply(F&& f, Key const& key, V&& value) const {
        static_assert(index == 0 || index == 1);
        if constexpr (index == 0) {
            return f(key, std::forward<V>(value));
        } else {
            return f(std::forward<V>(value), key);
        }
    }

    template <typename... Ts>
    auto const& get_value(std::tuple<Ts...> const& t) const {
        return t;
    }

    template <typename A, typename B>
    auto const& get_value(std::tuple<A, B> const& t) const {
        static_assert(index == 0 || index == 1);
        return std::get<1-index>(t);
    }
};

template <size_t index = 0>
struct PairExtractor {
    static_assert(index == 0 || index == 1);

    template <typename First, typename Second>
    auto const& operator()(std::pair<First, Second> const& t) const {
        return std::get<index>(t);
    }

    template <typename First, typename Second>
    auto const& operator()(First const& first, Second const& second) const {
        if constexpr (index == 0) {
            return first;
        } else {
            return second;
        }
    }

    template <typename... Args1, typename... Args2>
    auto const& operator()(std::piecewise_construct_t, std::tuple<Args1...> const& first_args, std::tuple<Args2...> const& second_args) const {
        if constexpr (index == 0) {
            static_assert(sizeof...(Args1) == 1);
            return std::get<0>(first_args);
        } else {
            static_assert(sizeof...(Args2) == 1);
            return std::get<0>(second_args);
        }
    }

    template <typename First, typename Second>
    auto const& get_key(std::pair<First, Second> const& t) const {
        return operator()(t);
    }

    template <typename First, typename Second>
    auto const& get_key(First const& first, Second const& second) const {
        return operator()(first, second);
    }

    template <typename... Args1, typename... Args2>
    auto const& get_key(std::piecewise_construct_t, std::tuple<Args1...> const& first_args, std::tuple<Args2...> const& second_args) const {
        return operator()(std::piecewise_construct, first_args, second_args);
    }

    template <typename F, typename Key, typename V>
    decltype(auto) apply(F&& f, Key const& key, V&& value) const {
        if constexpr (index == 0) {
            return f(key, std::forward<V>(value));
        } else {
            return f(std::forward<V>(value), key);
        }
    }

    template <typename First, typename Second>
    auto const& get_value(std::pair<First, Second> const& p) const {
        return std::get<1-index>(p);
    }
};

struct MinComparator {
    template <typename T, typename U>
    bool operator()(T const& t, U const& u) const {
        return t < u;
    }
};

struct MaxComparator {
    template <typename T, typename U>
    bool operator()(T const& t, U const& u) const {
        return u < t;
    }
};

enum struct Empty {
    empty
};

struct ValueExtractor {
    template <typename V>
    V const& operator()(V const& v) const {
        return v;
    }

    template <typename V>
    V const& get_key(V const& v) const {
        return v;
    }

    template <typename V>
    V const& get_value(V const& v) const {
        return v;
    }

    template <typename F, typename Key, typename V>
    decltype(auto) apply(F&& f, Key const&, V&& value) const {
        return f(std::forward<V>(value));
    }
};

template <typename T>
struct CastingExtractor {
    template <typename Value>
    T operator()(Value const& value) const {
        return static_cast<T>(value);
    }
};

template <typename Extractor, typename T>
struct WrappedCastingExtractor {
    static constexpr Extractor extractor{};

    template <typename... Args>
    T operator()(Args const&... args) const {
        return static_cast<T>(extractor(args...));
    }
};

template <typename NodeInfoType>
struct Replace {
    NodeInfoType delta{};
    // this is only used for fixing the iterator when erasing an element
    // if carry is true, the last element in the child was erased and the iterator should point at the first element of
    // the next child
    bool carry = false;
};

template <typename NodeInfoType>
struct Split {
    NodeInfoType left;
    NodeInfoType right;
    // this is only used for fixing the iterator when inserting an element
    // if new_element_left is true then the element that was inserted is in the left child, otherwise it is in the right
    bool new_element_left;

    template <typename LP, typename RP>
    Split(LP&& left_ptr, NodePtr<RP>&& right_ptr, bool left_changed, bool new_element_left) :
            left(std::forward<LP>(left_ptr), left_changed),
            right(std::move(right_ptr), true),
            new_element_left(new_element_left) {}
};

template <uint64_t i, uint64_t attempt>
inline constexpr int bits_required2() {
    if constexpr ((1ULL << attempt) - 1 >= i) {
        return attempt;
    } else {
        return bits_required2<i, attempt + 1>();
    }
}

template <uint64_t i>
inline constexpr int bits_required() {
    return bits_required2<i, 0>();
}

template<typename, typename = void>
struct IsTreeIterator : std::false_type {};

template<typename T>
struct IsTreeIterator<T, std::void_t<decltype(std::declval<T>().iter), decltype(std::declval<T>().leaf)>> : std::true_type {};

template<typename, typename, typename = void>
struct IsIndexedTree : std::false_type {};

template<typename T, typename It>
struct IsIndexedTree<T, It, std::void_t<decltype(std::declval<T>().order(std::declval<It>()))>> : std::true_type {};

template<typename, typename = void>
struct IsTransientTree : std::true_type {};

template<typename T>
struct IsTransientTree<T, std::void_t<decltype(std::declval<T>().transient())>> : std::false_type {};

enum struct DuplicatePolicy {
    replace,
    ignore,
    insert
};

} //end namespace detail
using detail::TupleExtractor;
using detail::PairExtractor;
using detail::ValueExtractor;
} //end namespace bpptree
