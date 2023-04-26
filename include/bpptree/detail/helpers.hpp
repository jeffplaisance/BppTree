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

#if defined(__cpp_lib_hardware_interference_size)
using std::hardware_constructive_interference_size;
#else
static constexpr size_t hardware_constructive_interference_size = 64;
#endif

using IndexType = std::int_fast32_t;

//a std::array that doesn't cause warnings when you index into it with signed integers when -Wsign-conversion is enabled
template <typename T, size_t N>
struct Array : std::array<T, N> {
    template <typename I, std::enable_if_t<std::is_integral_v<I>, bool> = true>
    decltype(auto) operator[](I const index) noexcept {
        return std::array<T, N>::operator[](static_cast<size_t const>(index));
    }

    template <typename I, std::enable_if_t<std::is_integral_v<I>, bool> = true>
    decltype(auto) operator[](I const index) const noexcept {
        return std::array<T, N>::operator[](static_cast<size_t const>(index));
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
    struct type {
        T operator()(Value const& value) const {
            return static_cast<T>(value);
        }
    };
};

template <typename Extractor, typename T>
struct WrappedCastingExtractor {
    static constexpr Extractor extractor{};

    template <typename... Args>
    T operator()(Args const&... args) const {
        return static_cast<T>(extractor(args...));
    }
};

template <typename Parent, typename PtrType>
struct NodeInfoBase : public Parent {
    NodePtr<PtrType> ptr{};
    bool ptrChanged = false;

    NodeInfoBase() = default;

    template <typename P>
    NodeInfoBase(P&& p, const bool changed) : ptr(), ptrChanged(changed) {
        if (changed) {
            this->ptr = std::forward<P>(p);
        }
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
    Split(LP&& leftPtr, NodePtr<RP>&& rightPtr, bool left_changed, bool new_element_left) :
            left(std::forward<LP>(leftPtr), left_changed),
            right(std::move(rightPtr), true),
            new_element_left(new_element_left) {}
};

template <uint64_t i, uint64_t attempt>
static constexpr int bits_required2() {
    if constexpr ((1ULL << attempt) - 1 >= i) {
        return attempt;
    } else {
        return bits_required2<i, attempt + 1>();
    }
}

template <uint64_t i>
static constexpr int bits_required() {
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

template<typename, typename, typename = void>
struct HasEq : std::false_type {};

template<typename A, typename B>
struct HasEq<A, B, std::void_t<decltype(std::declval<A>() == std::declval<B>())>> : std::true_type {};

template<typename, typename, typename = void>
struct HasLt : std::false_type {};

template<typename A, typename B>
struct HasLt<A, B, std::void_t<decltype(std::declval<A>() < std::declval<B>())>> : std::true_type {};

template<typename, typename, typename = void>
struct HasGt : std::false_type {};

template<typename A, typename B>
struct HasGt<A, B, std::void_t<decltype(std::declval<A>() > std::declval<B>())>> : std::true_type {};

template<typename, typename, typename = void>
struct HasLte : std::false_type {};

template<typename A, typename B>
struct HasLte<A, B, std::void_t<decltype(std::declval<A>() <= std::declval<B>())>> : std::true_type {};

template<typename, typename, typename = void>
struct HasGte : std::false_type {};

template<typename A, typename B>
struct HasGte<A, B, std::void_t<decltype(std::declval<A>() >= std::declval<B>())>> : std::true_type {};

template<typename, typename, typename = void>
struct HasAdd : std::false_type {};

template<typename A, typename B>
struct HasAdd<A, B, std::void_t<decltype(std::declval<A>() + std::declval<B>())>> : std::true_type {};

template<typename, typename, typename = void>
struct HasSub : std::false_type {};

template<typename A, typename B>
struct HasSub<A, B, std::void_t<decltype(std::declval<A>() - std::declval<B>())>> : std::true_type {};

template<typename, typename, typename = void>
struct HasMul : std::false_type {};

template<typename A, typename B>
struct HasMul<A, B, std::void_t<decltype(std::declval<A>() * std::declval<B>())>> : std::true_type {};

template<typename, typename, typename = void>
struct HasDiv : std::false_type {};

template<typename A, typename B>
struct HasDiv<A, B, std::void_t<decltype(std::declval<A>() / std::declval<B>())>> : std::true_type {};

template<typename, typename, typename = void>
struct HasMod : std::false_type {};

template<typename A, typename B>
struct HasMod<A, B, std::void_t<decltype(std::declval<A>() % std::declval<B>())>> : std::true_type {};

template<typename, typename, typename = void>
struct HasXor : std::false_type {};

template<typename A, typename B>
struct HasXor<A, B, std::void_t<decltype(std::declval<A>() ^ std::declval<B>())>> : std::true_type {};

template<typename, typename, typename = void>
struct HasAnd : std::false_type {};

template<typename A, typename B>
struct HasAnd<A, B, std::void_t<decltype(std::declval<A>() & std::declval<B>())>> : std::true_type {};

template<typename, typename, typename = void>
struct HasOr : std::false_type {};

template<typename A, typename B>
struct HasOr<A, B, std::void_t<decltype(std::declval<A>() | std::declval<B>())>> : std::true_type {};

template<typename, typename, typename = void>
struct HasLeftShift : std::false_type {};

template<typename A, typename B>
struct HasLeftShift<A, B, std::void_t<decltype(std::declval<A>() << std::declval<B>())>> : std::true_type {};

template<typename, typename, typename = void>
struct HasRightShift : std::false_type {};

template<typename A, typename B>
struct HasRightShift<A, B, std::void_t<decltype(std::declval<A>() >> std::declval<B>())>> : std::true_type {};

template <typename T>
struct ProxyOperators {
private:
    T& self() & { return static_cast<T&>(*this); }
    T&& self() && { return static_cast<T&&>(*this); }
    T const& self() const& { return static_cast<T const&>(*this); }
    T const&& self() const&& { return static_cast<T const&&>(*this); }
public:
    template <typename U>
    decltype(auto) operator+=(U const& u) {
        return self().invoke_compound_assignment([&u](auto const& v){ return v + u; });
    }

    template <typename U>
    decltype(auto) operator-=(U const& u) {
        return self().invoke_compound_assignment([&u](auto const& v){ return v - u; });
    }

    template <typename U>
    decltype(auto) operator*=(U const& u) {
        return self().invoke_compound_assignment([&u](auto const& v){ return v * u; });
    }

    template <typename U>
    decltype(auto) operator/=(U const& u) {
        return self().invoke_compound_assignment([&u](auto const& v){ return v / u; });
    }

    template <typename U>
    decltype(auto) operator%=(U const& u) {
        return self().invoke_compound_assignment([&u](auto const& v){ return v % u; });
    }

    template <typename U>
    decltype(auto) operator^=(U const& u) {
        return self().invoke_compound_assignment([&u](auto const& v){ return v ^ u; });
    }

    template <typename U>
    decltype(auto) operator&=(U const& u) {
        return self().invoke_compound_assignment([&u](auto const& v){ return v & u; });
    }

    template <typename U>
    decltype(auto) operator|=(U const& u) {
        return self().invoke_compound_assignment([&u](auto const& v){ return v | u; });
    }

    template <typename U>
    decltype(auto) operator>>=(U const& u) {
        return self().invoke_compound_assignment([&u](auto const& v){ return v >> u; });
    }

    template <typename U>
    decltype(auto) operator<<=(U const& u) {
        return self().invoke_compound_assignment([&u](auto const& v){ return v << u; });
    }

    template <typename = void>
    decltype(auto) operator++() {
        return self().invoke_pre([](auto& v){ return ++v; });
    }

    template <typename = void>
    decltype(auto) operator--() {
        return self().invoke_pre([](auto& v){ return --v; });
    }

    template <typename = void>
    decltype(auto) operator++(int) { // NOLINT(cert-dcl21-cpp)
        return self().invoke_post([](auto& v){ return v++; });
    }

    template <typename = void>
    decltype(auto) operator--(int) { // NOLINT(cert-dcl21-cpp)
        return self().invoke_post([](auto& v){ return v--; });
    }
private:
    template <typename U>
    static decltype(auto) get_if_proxy(U const& u) {
        if constexpr (std::is_same_v<U, T>) {
            return u.get();
        } else {
            return u;
        }
    }
public:
    template <typename A, typename B,
            std::enable_if_t<
                    !HasEq<A const&, B const&>::value &&
                    (std::is_same_v<A, T> || std::is_same_v<B, T>), bool> = true>
    [[nodiscard]] friend bool operator==(A const& a, B const& b) {
        return get_if_proxy(a) == get_if_proxy(b);
    }

    template <typename A, typename B,
            std::enable_if_t<
                    !HasLt<A const&, B const&>::value &&
                    (std::is_same_v<A, T> || std::is_same_v<B, T>), bool> = true>
    [[nodiscard]] friend bool operator<(A const& a, B const& b) {
        return get_if_proxy(a) < get_if_proxy(b);
    }

    template <typename A, typename B,
            std::enable_if_t<
                    !HasGt<A const&, B const&>::value &&
                    (std::is_same_v<A, T> || std::is_same_v<B, T>), bool> = true>
    [[nodiscard]] friend bool operator>(A const& a, B const& b) {
        return get_if_proxy(a) > get_if_proxy(b);
    }

    template <typename A, typename B,
            std::enable_if_t<
                    !HasLte<A const&, B const&>::value &&
                    (std::is_same_v<A, T> || std::is_same_v<B, T>), bool> = true>
    [[nodiscard]] friend bool operator<=(A const& a, B const& b) {
        return get_if_proxy(a) <= get_if_proxy(b);
    }

    template <typename A, typename B,
            std::enable_if_t<
                    !HasGte<A const&, B const&>::value &&
                    (std::is_same_v<A, T> || std::is_same_v<B, T>), bool> = true>
    [[nodiscard]] friend bool operator>=(A const& a, B const& b) {
        return get_if_proxy(a) >= get_if_proxy(b);
    }

    template <typename A, typename B,
            std::enable_if_t<
                    !HasAdd<A const&, B const&>::value &&
                    (std::is_same_v<A, T> || std::is_same_v<B, T>), bool> = true>
    [[nodiscard]] friend decltype(auto) operator+(A const& a, B const& b) {
        return get_if_proxy(a) + get_if_proxy(b);
    }

    template <typename A, typename B,
            std::enable_if_t<
                    !HasSub<A const&, B const&>::value &&
                    (std::is_same_v<A, T> || std::is_same_v<B, T>), bool> = true>
    [[nodiscard]] friend decltype(auto) operator-(A const& a, B const& b) {
        return get_if_proxy(a) - get_if_proxy(b);
    }

    template <typename A, typename B,
            std::enable_if_t<
                    !HasMul<A const&, B const&>::value &&
                    (std::is_same_v<A, T> || std::is_same_v<B, T>), bool> = true>
    [[nodiscard]] friend decltype(auto) operator*(A const& a, B const& b) {
        return get_if_proxy(a) * get_if_proxy(b);
    }

    template <typename A, typename B,
            std::enable_if_t<
                    !HasDiv<A const&, B const&>::value &&
                    (std::is_same_v<A, T> || std::is_same_v<B, T>), bool> = true>
    [[nodiscard]] friend decltype(auto) operator/(A const& a, B const& b) {
        return get_if_proxy(a) / get_if_proxy(b);
    }

    template <typename A, typename B,
            std::enable_if_t<
                    !HasMod<A const&, B const&>::value &&
                    (std::is_same_v<A, T> || std::is_same_v<B, T>), bool> = true>
    [[nodiscard]] friend decltype(auto) operator%(A const& a, B const& b) {
        return get_if_proxy(a) % get_if_proxy(b);
    }

    template <typename A, typename B,
            std::enable_if_t<
                    !HasXor<A const&, B const&>::value &&
                    (std::is_same_v<A, T> || std::is_same_v<B, T>), bool> = true>
    [[nodiscard]] friend decltype(auto) operator^(A const& a, B const& b) {
        return get_if_proxy(a) ^ get_if_proxy(b);
    }

    template <typename A, typename B,
            std::enable_if_t<
                    !HasAnd<A const&, B const&>::value &&
                    (std::is_same_v<A, T> || std::is_same_v<B, T>), bool> = true>
    [[nodiscard]] friend decltype(auto) operator&(A const& a, B const& b) {
        return get_if_proxy(a) & get_if_proxy(b);
    }

    template <typename A, typename B,
            std::enable_if_t<
                    !HasOr<A const&, B const&>::value &&
                    (std::is_same_v<A, T> || std::is_same_v<B, T>), bool> = true>
    [[nodiscard]] friend decltype(auto) operator|(A const& a, B const& b) {
        return get_if_proxy(a) | get_if_proxy(b);
    }

    template <typename A, typename B,
            std::enable_if_t<
                    !HasLeftShift<A const&, B const&>::value &&
                    (std::is_same_v<A, T> || std::is_same_v<B, T>), bool> = true>
    [[nodiscard]] friend decltype(auto) operator<<(A const& a, B const& b) {
        return get_if_proxy(a) << get_if_proxy(b);
    }

    template <typename A, typename B,
            std::enable_if_t<
                    !HasRightShift<A const&, B const&>::value &&
                    (std::is_same_v<A, T> || std::is_same_v<B, T>), bool> = true>
    [[nodiscard]] friend decltype(auto) operator>>(A const& a, B const& b) {
        return get_if_proxy(a) >> get_if_proxy(b);
    }
};

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
