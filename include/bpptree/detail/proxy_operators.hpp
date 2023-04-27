//
// B++ Tree: A B+ Tree library written in C++
// Copyright (C) 2023 Jeff Plaisance
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#pragma once

#include "helpers.hpp"

namespace bpptree::detail {

template <typename, typename, typename = void>
struct HasEq : std::false_type {
};

template <typename A, typename B>
struct HasEq<A, B, std::void_t<decltype(std::declval<A>() == std::declval<B>())>> : std::true_type {
};

template <typename, typename, typename = void>
struct HasLt : std::false_type {
};

template <typename A, typename B>
struct HasLt<A, B, std::void_t<decltype(std::declval<A>() < std::declval<B>())>> : std::true_type {
};

template <typename, typename, typename = void>
struct HasGt : std::false_type {
};

template <typename A, typename B>
struct HasGt<A, B, std::void_t<decltype(std::declval<A>() > std::declval<B>())>> : std::true_type {
};

template <typename, typename, typename = void>
struct HasLte : std::false_type {
};

template <typename A, typename B>
struct HasLte<A, B, std::void_t<decltype(std::declval<A>() <= std::declval<B>())>> : std::true_type {
};

template <typename, typename, typename = void>
struct HasGte : std::false_type {
};

template <typename A, typename B>
struct HasGte<A, B, std::void_t<decltype(std::declval<A>() >= std::declval<B>())>> : std::true_type {
};

template <typename, typename, typename = void>
struct HasAdd : std::false_type {
};

template <typename A, typename B>
struct HasAdd<A, B, std::void_t<decltype(std::declval<A>() + std::declval<B>())>> : std::true_type {
};

template <typename, typename, typename = void>
struct HasSub : std::false_type {
};

template <typename A, typename B>
struct HasSub<A, B, std::void_t<decltype(std::declval<A>() - std::declval<B>())>> : std::true_type {
};

template <typename, typename, typename = void>
struct HasMul : std::false_type {
};

template <typename A, typename B>
struct HasMul<A, B, std::void_t<decltype(std::declval<A>() * std::declval<B>())>> : std::true_type {
};

template <typename, typename, typename = void>
struct HasDiv : std::false_type {
};

template <typename A, typename B>
struct HasDiv<A, B, std::void_t<decltype(std::declval<A>() / std::declval<B>())>> : std::true_type {
};

template <typename, typename, typename = void>
struct HasMod : std::false_type {
};

template <typename A, typename B>
struct HasMod<A, B, std::void_t<decltype(std::declval<A>() % std::declval<B>())>> : std::true_type {
};

template <typename, typename, typename = void>
struct HasXor : std::false_type {
};

template <typename A, typename B>
struct HasXor<A, B, std::void_t<decltype(std::declval<A>() ^ std::declval<B>())>> : std::true_type {
};

template <typename, typename, typename = void>
struct HasAnd : std::false_type {
};

template <typename A, typename B>
struct HasAnd<A, B, std::void_t<decltype(std::declval<A>() & std::declval<B>())>> : std::true_type {
};

template <typename, typename, typename = void>
struct HasOr : std::false_type {
};

template <typename A, typename B>
struct HasOr<A, B, std::void_t<decltype(std::declval<A>() | std::declval<B>())>> : std::true_type {
};

template <typename, typename, typename = void>
struct HasLeftShift : std::false_type {
};

template <typename A, typename B>
struct HasLeftShift<A, B, std::void_t<decltype(std::declval<A>() << std::declval<B>())>> : std::true_type {
};

template <typename, typename, typename = void>
struct HasRightShift : std::false_type {
};

template <typename A, typename B>
struct HasRightShift<A, B, std::void_t<decltype(std::declval<A>() >> std::declval<B>())>> : std::true_type {
};

template <typename T>
struct ProxyOperators {
private:
    T& self()& { return static_cast<T&>(*this); }

    T&& self()&& { return static_cast<T&&>(*this); }

    T const& self() const& { return static_cast<T const&>(*this); }

    T const&& self() const&& { return static_cast<T const&&>(*this); }

public:
    template <typename U>
    decltype(auto) operator+=(U const& u) {
        return self().invoke_compound_assignment([&u](auto const& v) { return v + u; });
    }

    template <typename U>
    decltype(auto) operator-=(U const& u) {
        return self().invoke_compound_assignment([&u](auto const& v) { return v - u; });
    }

    template <typename U>
    decltype(auto) operator*=(U const& u) {
        return self().invoke_compound_assignment([&u](auto const& v) { return v * u; });
    }

    template <typename U>
    decltype(auto) operator/=(U const& u) {
        return self().invoke_compound_assignment([&u](auto const& v) { return v / u; });
    }

    template <typename U>
    decltype(auto) operator%=(U const& u) {
        return self().invoke_compound_assignment([&u](auto const& v) { return v % u; });
    }

    template <typename U>
    decltype(auto) operator^=(U const& u) {
        return self().invoke_compound_assignment([&u](auto const& v) { return v ^ u; });
    }

    template <typename U>
    decltype(auto) operator&=(U const& u) {
        return self().invoke_compound_assignment([&u](auto const& v) { return v & u; });
    }

    template <typename U>
    decltype(auto) operator|=(U const& u) {
        return self().invoke_compound_assignment([&u](auto const& v) { return v | u; });
    }

    template <typename U>
    decltype(auto) operator>>=(U const& u) {
        return self().invoke_compound_assignment([&u](auto const& v) { return v >> u; });
    }

    template <typename U>
    decltype(auto) operator<<=(U const& u) {
        return self().invoke_compound_assignment([&u](auto const& v) { return v << u; });
    }

    template <typename = void>
    decltype(auto) operator++() {
        return self().invoke_pre([](auto& v) { return ++v; });
    }

    template <typename = void>
    decltype(auto) operator--() {
        return self().invoke_pre([](auto& v) { return --v; });
    }

    template <typename = void>
    decltype(auto) operator++(int) { // NOLINT(cert-dcl21-cpp)
        return self().invoke_post([](auto& v) { return v++; });
    }

    template <typename = void>
    decltype(auto) operator--(int) { // NOLINT(cert-dcl21-cpp)
        return self().invoke_post([](auto& v) { return v--; });
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
} //end namespace bpptree::detail