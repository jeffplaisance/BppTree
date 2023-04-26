//
// B++ Tree: A B+ Tree library written in C++
// Copyright (C) 2023 Jeff Plaisance
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#pragma once

#include <array>
#include <type_traits>
#include "bpptree/detail/helpers.hpp"
#include "bpptree/detail/summed_detail.hpp"

namespace bpptree {
namespace detail {

/**
 * Summed mixin adds prefix sum support to a B++ tree in O(log N) time
 * sum() returns the sum over the entire tree
 * sum_inclusive(it) returns the sum up to and including the element pointed to by the iterator 'it'
 * sum_exclusive(it) returns the sum up to but not including the element pointed to by the iterator 'it'
 * If you want to sum between two iterators, do sum_exclusive(it2)-sum_exclusive(it1)
 * sum_lower_bound(target) returns an iterator pointing to the first element for which sum_inclusive(it) >= target
 * @tparam Value the value type of the B++ tree
 * @tparam Extractor a class which implements operator()(Value) to return the value to be summed
 */
template <typename Value, typename Extractor = ValueExtractor>
struct Summed {
private:
    using SumType = std::remove_cv_t<std::remove_reference_t<decltype(std::declval<Extractor>()(std::declval<Value>()))>>;
public:
    template <typename Parent>
    using LeafNode = SummedLeafNode<Parent, SumType, Extractor>;

    template <typename Parent, auto internal_size>
    using InternalNode = SummedInternalNode<Parent, SumType, internal_size>;

    template <typename Parent>
    using NodeInfo = SummedNodeInfo<Parent, SumType>;

    template <typename Parent>
    struct Shared : public Parent {
        template <typename... Us>
        explicit Shared(Us&&... us) noexcept : Parent(std::forward<Us>(us)...) {}

        /**
         * @return an iterator pointing to the first element such that sum_inclusive(it) >= target
         */
        [[nodiscard]] typename Parent::iterator sum_lower_bound(SumType target) {
            typename Parent::iterator ret(this->self());
            std::as_const(this->self()).dispatch([target, &ret](auto& root) {
                root->seek_index_forward(ret, target);
            });
            return ret;
        }

        /**
         * @return a const_iterator pointing to the first element such that sum_inclusive(it) >= target
         */
        [[nodiscard]] typename Parent::const_iterator sum_lower_bound_const(SumType target) const {
            typename Parent::const_iterator ret(this->self());
            std::as_const(this->self()).dispatch([target, &ret](auto& root) {
                root->seek_index_forward(ret, target);
            });
            return ret;
        }

        /**
         * @return a const_iterator pointing to the first element such that sum_inclusive(it) >= target
         */
        [[nodiscard]] typename Parent::const_iterator sum_lower_bound(SumType target) const {
            return sum_lower_bound_const(target);
        }

        /**
         * @return the sum of all elements in the tree
         */
        [[nodiscard]] SumType sum() const {
            return this->self().dispatch(
                [](auto const& root) { return root->sum(); }
            );
        }

        /**
         * @return the sum of all elements up to and including the element pointed to by the iterator it
         */
        template <typename It>
        [[nodiscard]] SumType sum_inclusive(It const& it) const {
            SumType ret{};
            this->self().dispatch(
                [&it, &ret](auto const& root) {
                    root->sum_inclusive(it.iter, ret);
                }
            );
            return ret;
        }

        /**
         * @return the sum of all elements up to but not including the element pointed to by the iterator it
         */
        template <typename It>
        [[nodiscard]] SumType sum_exclusive(It const& it) const {
            SumType ret{};
            this->self().dispatch(
                    [&it, &ret](auto const& root) {
                        root->sum_exclusive(it.iter, ret);
                    }
            );
            return ret;
        }
    };

    template <typename Parent>
    struct Transient : public Parent {
        template <typename... Us>
        explicit Transient(Us&&... us) noexcept : Parent(std::forward<Us>(us)...) {}
    };

    template <typename Parent>
    struct Persistent : public Parent {
        template <typename... Us>
        explicit Persistent(Us&&... us) noexcept : Parent(std::forward<Us>(us)...) {}
    };
};

template <typename Extractor = ValueExtractor>
struct SummedBuilder {
    template <typename Value>
    using build = Summed<Value, Extractor>;
};
} //end namespace detail
using detail::Summed;
using detail::SummedBuilder;
} //end namespace bpptree
