//
// B++ Tree: A B+ Tree library written in C++
// Copyright (C) 2023 Jeff Plaisance
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#pragma once

#include <new>
#include <memory>

namespace bpptree::detail {

/**
 * Similar to std::array but contents are not default initialized.
 * The expectation is that the user tracks an index externally below which all elements have been initialized
 * and at or above no elements have been initialized.
 */
template <typename T, size_t n>
struct UninitializedArray {
private:
    // storage for the array is implemented as a union with a single member which is an array of T of size n.
    // a union is used here because it prevents default construction of the array members.
    // i did not use an array of unions containing a single T because there is no standard compliant way as far as i
    // can tell to convert that to a T* that can be used to index into the array.
    // useful reference: How to Hold a T - CJ Johnson - CppCon 2019 https://www.youtube.com/watch?v=WX8FsrUbLjc
    union U {
        T data[n];

        U() noexcept {};
        ~U() {};
    };

    U un;

    template <typename... Us>
    static constexpr bool is_assignable_from() {
        if constexpr (sizeof...(Us) == 1) {
            if constexpr (std::is_assignable_v<T&, Us&&...>) {
                return true;
            }
        }
        return false;
    }
public:
    UninitializedArray() = default;

    UninitializedArray(UninitializedArray const& other, size_t length) {
        std::uninitialized_copy_n(other.un.data, length, un.data);
    }

    UninitializedArray(UninitializedArray const& other) = delete;
    UninitializedArray& operator=(UninitializedArray const& other) = delete;
    UninitializedArray(UninitializedArray && other) = delete;
    UninitializedArray& operator=(UninitializedArray && other) = delete;

    template <typename I, std::enable_if_t<std::is_integral_v<I>, bool> = true>
    T& operator[](I const index) {
        return un.data[index];
    }

    template <typename I, std::enable_if_t<std::is_integral_v<I>, bool> = true>
    T const& operator[](I const index) const {
        return un.data[index];
    }

    template <typename I, std::enable_if_t<std::is_integral_v<I>, bool> = true>
    T&& move(I const index) {
        return std::move((*this)[index]);
    }

    template <typename I, std::enable_if_t<std::is_integral_v<I>, bool> = true>
    void destruct(I const index) noexcept {
        (*this)[index].~T();
    }

    template <typename I, typename L, typename U,
            std::enable_if_t<std::is_integral_v<I>, bool> = true,
            std::enable_if_t<std::is_integral_v<L>, bool> = true>
    T& set(I const index, L const length, U&& u) {
        // can only assign to uninitialized memory if the type is trivial, otherwise the lifetime of the object has
        // not yet begun and a constructor must be called
        if constexpr (std::is_trivial_v<T> && std::is_trivially_assignable_v<T&, U&&>) {
            (*this)[index] = std::forward<U>(u);
            return (*this)[index];
        }
        if (static_cast<size_t const>(index) < static_cast<size_t const>(length)) {
            (*this)[index] = std::forward<U>(u);
            return (*this)[index];
        }
        return *new(un.data + index) T(std::forward<U>(u));
    }

    template <typename I, std::enable_if_t<std::is_integral_v<I>, bool> = true, typename... Us>
    T& initialize(I const index, Us&&... us) {
        return *new(un.data + index) T(std::forward<Us>(us)...);
    }

    template <typename I, typename L,
            std::enable_if_t<std::is_integral_v<I>, bool> = true,
            std::enable_if_t<std::is_integral_v<L>, bool> = true,
            typename... Us>
    T& emplace(I const index, L const length, Us&&... us) noexcept {
        // can only assign to uninitialized memory if the type is trivial, otherwise the lifetime of the object has
        // not yet begun and a constructor must be called
        if constexpr (is_assignable_from<Us&&...>() && std::is_trivial_v<T>) {
            if constexpr (std::is_trivially_assignable_v<T&, Us&&...>) {
                (*this)[index] = (..., std::forward<Us>(us));
                return (*this)[index];
            }
        }
        if (static_cast<size_t const>(index) < static_cast<size_t const>(length)) {
            if constexpr (is_assignable_from<Us&&...>()) {
                (*this)[index] = (..., std::forward<Us>(us));
                return (*this)[index];
            }
            (*this)[index].~T();
        }
        return *new(un.data + index) T(std::forward<Us>(us)...);
    }

    template <typename I, std::enable_if_t<std::is_integral_v<I>, bool> = true, typename... Us>
    T& emplace_unchecked(I const index, Us&&... us) noexcept {
        if constexpr (is_assignable_from<Us&&...>()) {
            (*this)[index] = (..., std::forward<Us>(us));
            return (*this)[index];
        }
        (*this)[index].~T();
        return *new(un.data + index) T(std::forward<Us>(us)...);
    }

    T* begin() {
        return &un.data[0];
    }

    T* end() {
        return &un.data[n];
    }

    T const* cbegin() const {
        return &un.data[0];
    }

    T const* cend() const {
        return &un.data[n];
    }

    T const* begin() const {
        return cbegin();
    }

    T const* end() const {
        return cend();
    }
};
} //end namespace bpptree::detail
