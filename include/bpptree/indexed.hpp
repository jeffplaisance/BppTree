//
// B++ Tree: A B+ Tree library written in C++
// Copyright (C) 2023 Jeff Plaisance
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#pragma once

#include <array>
#include <tuple>
#include <cstdint>

#include "bpptree/detail/indexed_detail.hpp"
#include "bpptree/detail/proxy_operators.hpp"

namespace bpptree {
namespace detail {

/**
 * Indexed mixin adds support for indexing into a B++ tree by an integer index
 * Supports lookup, assign, insert, update, and erase by index in O(log N) time
 * Also supports getting the index of an element from an iterator
 * Given two iterators a and b from an indexed tree, a-b is O(log N) (it is O(N) for non-indexed trees)
 * @tparam Value value type of tree
 * @tparam SizeType type to use for index
 */
template <typename Value, typename SizeType = size_t>
struct Indexed {
    static constexpr size_t sizeof_hint() {
        return sizeof(SizeType);
    }

    template <typename Parent>
    using LeafNode = IndexedLeafNode<Parent, Value, SizeType>;

    template <typename Parent, auto internal_size>
    using InternalNode = IndexedInternalNode<Parent, Value, SizeType, internal_size>;

    template <typename Parent>
    using NodeInfo = IndexedNodeInfo<Parent, SizeType>;

    template <typename Parent>
    struct Shared : public Parent {
        template <typename... Us>
        explicit Shared(Us&&... us) noexcept : Parent(std::forward<Us>(us)...) {}

        /**
         * @return a const reference to the value at index
         */
        [[nodiscard]] Value const& at_index(SizeType index) const {
            return this->self().dispatch(
                [index](auto const& root) -> Value const& { return root->at_index(index + 1); }
            );
        }

    private:
        template <typename It>
        void seek_index_internal(SizeType index, It& it) const {
            this->self().dispatch([index, &it](auto& root) {
                root->seek_index(it, index + 1);
            });
        }

    public:
        /**
         * @return a forward iterator pointing to the element at index
         */
        [[nodiscard]] typename Parent::iterator find_index(SizeType index) {
            typename Parent::iterator ret(this->self());
            seek_index_internal(index, ret);
            return ret;
        }

        /**
         * @return a forward const_iterator pointing to the element at index
         */
        [[nodiscard]] typename Parent::const_iterator find_index_const(SizeType index) const {
            typename Parent::const_iterator ret(this->self());
            seek_index_internal(index, ret);
            return ret;
        }

        /**
         * @return a forward const_iterator pointing to the element at index
         */
        [[nodiscard]] typename Parent::const_iterator find_index(SizeType index) const {
            return find_index_const(index);
        }

        /**
         * @tparam It the type of iterator (supports const and non-const iterators as well as forward and reverse)
         * @param it an iterator of type It
         * @return the index of the element pointed to by the iterator
         */
        template <typename It>
        [[nodiscard]] SizeType order(It const& it) const {
            SizeType ret{};
            this->self().dispatch(
                    [&it, &ret](auto const& root) {
                        root->order(it.iter, ret);
                    }
            );
            return ret;
        }
    };

    template <typename Parent>
    struct Transient : public Parent {
        template <typename... Us>
        explicit Transient(Us&&... us) noexcept : Parent(std::forward<Us>(us)...) {}

    protected:
        template<typename T>
        using Modify = typename Parent::template Modify<T>;

    public:
        /**
         * Constructs an element in place at index using args, moving all elements starting from index back by one
         */
        template <typename... Args>
        void insert_index(SizeType index, Args&&... args) {
            this->self().dispatch(
                Modify<Insert>(),
                [](auto const& node, auto const& search_val) {
                    return node.insertion_index(search_val);
                },
                index,
                std::forward<Args>(args)...
            );
        }

        /**
         * Constructs and element in place at index using args, replacing the element that is currently there
         */
        template <typename... Args>
        void assign_index(SizeType index, Args&&... args) {
            this->self().dispatch(
                Modify<Assign>(),
                [](auto const& node, auto const& search_val) {
                    return node.find_index(search_val);
                },
                index + 1,
                std::forward<Args>(args)...
            );
        }

        /**
         * Erases the element at index, moving all elements after it forward by one
         */
        void erase_index(SizeType index) {
            this->self().dispatch(
                Modify<Erase>(),
                [](auto const& node, auto const& search_val) {
                    return node.find_index(search_val);
                },
                index + 1
            );
        }

        /**
         * Apply updater function to element at index, and overwrites the element at index with the returned result
         * @param updater function that takes a Value const& and returns a Value (or something convertible to a Value)
         */
        template <typename U>
        void update_index(SizeType index, U&& updater) {
            this->self().dispatch(
                    Modify<Update>(),
                    [](auto const& node, auto const& search_val) {
                        return node.find_index(search_val);
                    },
                    index + 1,
                    updater
            );
        }

        struct ProxyRef : public ProxyOperators<ProxyRef> {
        private:
            friend struct ProxyOperators<ProxyRef>;

            Transient& tree;
            SizeType const& index;

            ProxyRef(const ProxyRef& other) = default;
            ProxyRef(ProxyRef&& other) noexcept = default;
            ProxyRef& operator=(const ProxyRef& other) = default;
            ProxyRef& operator=(ProxyRef&& other) noexcept = default;

            template <typename F>
            ProxyRef& invoke_compound_assignment(F&& f) {
                tree.update_index(index, f);
                return *this;
            }

            template <typename F>
            ProxyRef& invoke_pre(F&& f) {
                tree.update_index(index, [&f](auto const& v){
                    Value copy(v);
                    f(copy);
                    return copy;
                });
                return *this;
            }

            template <typename F>
            Value invoke_post(F&& f) {
                Value ret;
                tree.update_index(index, [&f, &ret](auto const& v){
                    Value copy(v);
                    ret = f(copy);
                    return copy;
                });
                return ret;
            }
        public:
            ProxyRef(Transient& tree, SizeType const& index) noexcept : tree(tree), index(index) {}
            ~ProxyRef() noexcept = default;

            ProxyRef& operator=(Value const& value) {
                tree.assign_index(index, value);
                return *this;
            }

            operator Value const&() const { //NOLINT
                return tree.at_index(index);
            }

            [[nodiscard]] Value const& get() const {
                return tree.at_index(index);
            }
        };

        /**
         * @return a ProxyRef to the value at index which supports assignment, compound assignment, pre- and post-
         * increment and decrement, all binary operators, and implicit conversion to const reference to Value.
         */
        [[nodiscard]] ProxyRef operator[](SizeType const& index) {
            return ProxyRef(*this, index);
        }

        /**
         * @return a const reference to the value at index
         */
        [[nodiscard]] Value const& operator[](SizeType const& index) const {
            return this->at_index(index);
        }
    };

    template <typename Parent>
    struct Persistent : public Parent {
        template <typename... Us>
        explicit Persistent(Us&&... us) noexcept : Parent(std::forward<Us>(us)...) {}

        /**
         * Makes a transient copy of tree, calls insert_index on it with index and args, makes a persistent copy of the
         * transient tree, and returns it.
         * @return A copy of this tree that has been modified by calling insert_index with index and args
         */
        template <typename... Args>
        [[nodiscard]] auto insert_index(SizeType index, Args&&... args) const {
            auto transient = this->self().transient();
            transient.insert_index(index, std::forward<Args>(args)...);
            return std::move(transient).persistent();
        }

        /**
         * Makes a transient copy of tree, calls assign_index on it with index and args, makes a persistent copy of the
         * transient tree, and returns it.
         * @return A copy of this tree that has been modified by calling assign_index with index and args
         */
        template <typename... Args>
        [[nodiscard]] auto assign_index(SizeType index, Args&&... args) const {
            auto transient = this->self().transient();
            transient.assign_index(index, std::forward<Args>(args)...);
            return std::move(transient).persistent();
        }

        /**
         * Makes a transient copy of tree, calls erase_index on it with index, makes a persistent copy of the transient
         * tree, and returns it.
         * @return A copy of this tree that has been modified by calling erase_index with index
         */
        [[nodiscard]] auto erase_index(SizeType index) const {
            auto transient = this->self().transient();
            transient.erase_index(index);
            return std::move(transient).persistent();
        }

        /**
         * Makes a transient copy of tree, calls update_index on it with index and updater, makes a persistent copy of
         * the transient tree, and returns it.
         * @return A copy of this tree that has been modified by calling update_index with index and updater
         */
        template <typename U>
        [[nodiscard]] auto update_index(SizeType index, U&& updater) const {
            auto transient = this->self().transient();
            transient.update_index(index, updater);
            return std::move(transient).persistent();
        }

        /**
         * @return a const reference to the value at index
         */
        [[nodiscard]] Value const& operator[](SizeType const& index) const {
            return this->at_index(index);
        }
    };
};

template <typename SizeType = size_t>
struct IndexedBuilder {
    template <typename Value>
    using build = Indexed<Value, SizeType>;
};

template <
        typename Value,
        typename SizeType = size_t,
        int leaf_node_bytes_v = 512,
        int internal_node_bytes_v = 512,
        int depth_limit_v = 16>
struct BppTreeVector {

    template <typename T>
    using size_type = BppTreeVector<Value, T, leaf_node_bytes_v, internal_node_bytes_v, depth_limit_v>;

    template <int l>
    using leaf_node_bytes = BppTreeVector<Value, SizeType, l, internal_node_bytes_v, depth_limit_v>;

    template <int i>
    using internal_node_bytes = BppTreeVector<Value, SizeType, leaf_node_bytes_v, i, depth_limit_v>;

    template <int d>
    using depth_limit = BppTreeVector<Value, SizeType, leaf_node_bytes_v, internal_node_bytes_v, d>;

    template <typename... Args>
    using mixins = typename BppTree<
            Value,
            leaf_node_bytes_v,
            internal_node_bytes_v,
            depth_limit_v>
    ::template mixins<IndexedBuilder<SizeType>, Args...>;

    using Transient = typename mixins<>::Transient;

    using Persistent = typename mixins<>::Persistent;
};
} //end namespace detail
using detail::Indexed;
using detail::IndexedBuilder;
using detail::BppTreeVector;
} //end namespace bpptree
