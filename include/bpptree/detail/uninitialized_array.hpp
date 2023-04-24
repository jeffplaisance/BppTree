//
// B++ Tree: A B+ Tree library written in C++
// Copyright (C) 2023 Jeff Plaisance
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#pragma once

#include <new>
#include <cstring>
#include "common.hpp"

namespace bpptree::detail {

template <typename... Args> struct FirstElement;

template <>
struct FirstElement<> {
    using type = void;
};

template <typename First, typename... Rest>
struct FirstElement<First, Rest...> {
    using type = First;
};

template <typename... Args>
using FirstElementT = typename FirstElement<Args...>::type;

template <typename First>
decltype(auto) first_element(First&& first) {
    return std::forward<First>(first);
}

/**
 * Similar to std::array but contents are not default initialized.
 * The expectation is that the user tracks an index externally below which all elements have been initialized
 * and at or above no elements have been initialized.
 */
template <typename T, size_t n>
struct UninitializedArray {
    static_assert(sizeof(T) >= alignof(T)); //NOLINT
    alignas(T) std::array<std::byte, sizeof(T)*n> data;

    UninitializedArray() noexcept = default;

    UninitializedArray(UninitializedArray const& other, size_t length) noexcept {
        if constexpr (std::is_trivially_copy_constructible_v<T>) {
            std::memcpy(data.data(), other.data.data(), sizeof(T)*length);
        } else {
            T* base = reinterpret_cast<T*>(data.data());
            T const* other_base = reinterpret_cast<T const*>(other.data.data());
            for (size_t i = 0; i < length; ++i) {
                new(base + i) T(other_base[i]);
            }
        }
    }

    UninitializedArray(UninitializedArray const& other) noexcept = delete;
    UninitializedArray& operator=(UninitializedArray const& other) noexcept = delete;
    UninitializedArray(UninitializedArray && other) noexcept = delete;
    UninitializedArray& operator=(UninitializedArray && other) noexcept = delete;

    template <typename I, std::enable_if_t<std::is_integral_v<I>, bool> = true>
    T& operator[](I const index) noexcept {
        return reinterpret_cast<T*>(data.data())[static_cast<size_t const>(index)];
    }

    template <typename I, std::enable_if_t<std::is_integral_v<I>, bool> = true>
    T const& operator[](I const index) const noexcept {
        return reinterpret_cast<T const*>(data.data())[static_cast<size_t const>(index)];
    }

    template <typename I, std::enable_if_t<std::is_integral_v<I>, bool> = true>
    T&& move(I const index) noexcept {
        return std::move(reinterpret_cast<T*>(data.data())[static_cast<size_t const>(index)]);
    }

    template <typename I, std::enable_if_t<std::is_integral_v<I>, bool> = true>
    void destruct(I const index) noexcept {
        if constexpr (!std::is_trivially_destructible_v<T>) {
            reinterpret_cast<T*>(data.data())[static_cast<size_t const>(index)].~T();
        }
    }

    template <typename I, typename L, typename U,
            std::enable_if_t<std::is_integral_v<I>, bool> = true,
            std::enable_if_t<std::is_integral_v<L>, bool> = true>
    T& set(I const index, L const length, U&& u) noexcept {
        T& t = reinterpret_cast<T*>(data.data())[static_cast<size_t const>(index)];
        if constexpr (std::is_trivially_assignable_v<T&, U&&>) {
            return t = std::forward<U>(u);
        } else {
            if (static_cast<ssize>(index) < static_cast<ssize const>(length)) {
                return t = std::forward<U>(u);
            }
        }
        return *new(&t) T(std::forward<U>(u));
    }

    template <typename I, std::enable_if_t<std::is_integral_v<I>, bool> = true, typename... Us>
    T& initialize(I const index, Us&&... us) noexcept {
        T& t = reinterpret_cast<T*>(data.data())[static_cast<size_t const>(index)];
        return *new(&t) T(std::forward<Us>(us)...);
    }

    template <typename I, typename L,
            std::enable_if_t<std::is_integral_v<I>, bool> = true,
            std::enable_if_t<std::is_integral_v<L>, bool> = true,
            typename... Us>
    T& emplace(I const index, L const length, Us&&... us) noexcept {
        T& t = reinterpret_cast<T*>(data.data())[static_cast<size_t const>(index)];
        if constexpr (sizeof...(Us) == 1 && std::is_assignable_v<T&, FirstElementT<Us...>&&>) {
            if constexpr (std::is_trivially_assignable_v<T&, Us&&...>) {
                return t = first_element(std::forward<Us>(us)...);
            } else {
                if (static_cast<ssize const>(index) < static_cast<ssize const>(length)) {
                    return t = first_element(std::forward<Us>(us)...);
                }
            }
        } else if constexpr (!std::is_trivially_destructible_v<T>) {
            if (static_cast<ssize const>(index) < static_cast<ssize const>(length)) {
                t.~T();
            }
        }
        return *new(&t) T(std::forward<Us>(us)...);
    }

    template <typename I, std::enable_if_t<std::is_integral_v<I>, bool> = true, typename... Us>
    T& emplace_unchecked(I const index, Us&&... us) noexcept {
        T& t = reinterpret_cast<T*>(data.data())[static_cast<size_t const>(index)];
        if constexpr (sizeof...(Us) == 1 && std::is_assignable_v<T&, FirstElementT<Us...>&&>) {
            return t = first_element(std::forward<Us>(us)...);
        } else if constexpr (!std::is_trivially_destructible_v<T>) {
            t.~T();
        }
        return *new(&t) T(std::forward<Us>(us)...);
    }

    T* begin() {
        return reinterpret_cast<T*>(data.data());
    }

    T* end() {
        return reinterpret_cast<T*>(data.data()) + n;
    }

    T const* begin() const {
        return reinterpret_cast<T const*>(data.data());
    }

    T const* end() const {
        return reinterpret_cast<T const*>(data.data()) + n;
    }

    T const* cbegin() const {
        return reinterpret_cast<T const*>(data.data());
    }

    T const* cend() const {
        return reinterpret_cast<T const*>(data.data()) + n;
    }
};
} //end namespace bpptree::detail
