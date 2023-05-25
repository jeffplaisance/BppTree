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
private:
    union U {
        T t;

        U() noexcept {}; //NOLINT
        ~U() {};
    };

    static_assert(sizeof(T) == sizeof(U));
    static_assert(alignof(T) == alignof(U));
    static_assert(sizeof(T) >= alignof(T)); //NOLINT
    static_assert(sizeof(T) % alignof(T) == 0);

    U data[n];
public:
    UninitializedArray() = default;

    UninitializedArray(UninitializedArray const& other, size_t length) {
        if constexpr (std::is_trivially_copy_constructible_v<T>) {
            std::memcpy(static_cast<void*>(data), static_cast<void const*>(other.data), sizeof(T)*length);
        } else {
            for (size_t i = 0; i < length; ++i) {
                new(data + i) T(other[i]);
            }
        }
    }

    UninitializedArray(UninitializedArray const& other) = delete;
    UninitializedArray& operator=(UninitializedArray const& other) = delete;
    UninitializedArray(UninitializedArray && other) = delete;
    UninitializedArray& operator=(UninitializedArray && other) = delete;

    template <typename I, std::enable_if_t<std::is_integral_v<I>, bool> = true>
    T& operator[](I const index) {
        return *std::launder(&data[index].t);
    }

    template <typename I, std::enable_if_t<std::is_integral_v<I>, bool> = true>
    T const& operator[](I const index) const {
        return *std::launder(&data[index].t);
    }

    template <typename I, std::enable_if_t<std::is_integral_v<I>, bool> = true>
    T&& move(I const index) {
        return std::move((*this)[index]);
    }

    template <typename I, std::enable_if_t<std::is_integral_v<I>, bool> = true>
    void destruct(I const index) noexcept {
        if constexpr (!std::is_trivially_destructible_v<T>) {
            (*this)[index].~T();
        }
    }

    template <typename I, typename L, typename U,
            std::enable_if_t<std::is_integral_v<I>, bool> = true,
            std::enable_if_t<std::is_integral_v<L>, bool> = true>
    T& set(I const index, L const length, U&& u) {
        if constexpr (std::is_trivially_assignable_v<T&, U&&>) {
            return (*this)[index] = std::forward<U>(u);
        } else {
            if (static_cast<size_t>(index) < static_cast<size_t const>(length)) {
                return (*this)[index] = std::forward<U>(u);
            }
        }
        return *new(data + index) T(std::forward<U>(u));
    }

    template <typename I, std::enable_if_t<std::is_integral_v<I>, bool> = true, typename... Us>
    T& initialize(I const index, Us&&... us) {
        return *new(data + index) T(std::forward<Us>(us)...);
    }

    template <typename I, typename L,
            std::enable_if_t<std::is_integral_v<I>, bool> = true,
            std::enable_if_t<std::is_integral_v<L>, bool> = true,
            typename... Us>
    T& emplace(I const index, L const length, Us&&... us) {
        if constexpr (sizeof...(Us) == 1 && std::is_assignable_v<T&, FirstElementT<Us...>&&>) {
            if constexpr (std::is_trivially_assignable_v<T&, Us&&...>) {
                return (*this)[index] = first_element(std::forward<Us>(us)...);
            } else {
                if (static_cast<size_t const>(index) < static_cast<size_t const>(length)) {
                    return (*this)[index] = first_element(std::forward<Us>(us)...);
                }
            }
        } else if constexpr (!std::is_trivially_destructible_v<T>) {
            if (static_cast<size_t const>(index) < static_cast<size_t const>(length)) {
                (*this)[index].~T();
            }
        }
        return *new(data + index) T(std::forward<Us>(us)...);
    }

    template <typename I, std::enable_if_t<std::is_integral_v<I>, bool> = true, typename... Us>
    T& emplace_unchecked(I const index, Us&&... us) {
        if constexpr (sizeof...(Us) == 1 && std::is_assignable_v<T&, FirstElementT<Us...>&&>) {
            return (*this)[index] = first_element(std::forward<Us>(us)...);
        } else if constexpr (!std::is_trivially_destructible_v<T>) {
            (*this)[index].~T();
        }
        return *new(data + index) T(std::forward<Us>(us)...);
    }

    T* begin() {
        return std::launder(&data[0].t);
    }

    T* end() {
        return std::launder(&data[n].t);
    }

    T const* cbegin() const {
        return std::launder(&data[0].t);
    }

    T const* cend() const {
        return std::launder(&data[n].t);
    }

    T const* begin() const {
        return cbegin();
    }

    T const* end() const {
        return cend();
    }
};
} //end namespace bpptree::detail
