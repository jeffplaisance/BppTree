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
#include <stdexcept>
#include <optional>

#include "bpptree/detail/helpers.hpp"
#include "bpptree/detail/uninitialized_array.hpp"
#include "bpptree/detail/ordered_detail.hpp"
#include "bpptree/detail/proxy_operators.hpp"

namespace bpptree {
namespace detail {

/**
 * Ordered mixin adds lookup by key into a B++ tree. Values in the B++ tree must be ordered by key to use Ordered.
 * Supports lookup, lower_bound, upper_bound, assign, insert, update, and erase by key in O(log N) time
 * @tparam KeyValue the element type of the B++ tree
 * @tparam KeyValueExtractor a class which implements functions to get a key from a KeyValue, get a value from a KeyValue,
 * or forward a key and value to another function in the correct order
 * @tparam LessThan a class which implements operator()(A const& a, B const& b) to return true if a < b and false otherwise
 * @tparam BinarySearch std::true_type to use binary search within internal and leaf nodes, std::false_type to use
 * linear search. linear search is typically faster for value types, but binary search is likely to be faster if
 * comparing two keys requires an indirection or if the nodes are exceptionally large.
 */
template <typename KeyValue, typename KeyValueExtractor = PairExtractor<0>, typename LessThan = MinComparator, typename BinarySearch = std::false_type>
struct Ordered {
private:
    static constexpr KeyValueExtractor extractor{};

    static constexpr LessThan less_than{};

    static constexpr bool binary_search = BinarySearch::value;

    using Key = std::remove_cv_t<std::remove_reference_t<decltype(extractor.get_key(std::declval<KeyValue const&>()))>>;

    using GetValue = decltype(extractor.get_value(std::declval<KeyValue const&>()));

    using Value = std::remove_cv_t<std::remove_reference_t<GetValue>>;

    using Detail = OrderedDetail<KeyValue, KeyValueExtractor, LessThan, binary_search>;
public:
    template <typename Parent>
    using LeafNode = typename Detail::template LeafNode<Parent>;

    template <typename Parent, auto internal_size>
    using InternalNode = typename Detail::template InternalNode<Parent, internal_size>;

    template <typename Parent>
    using NodeInfo = typename Detail::template NodeInfo<Parent>;

    template <typename Parent>
    struct Shared : public Parent {
        template <typename... Us>
        explicit Shared(Us&&... us) noexcept : Parent(std::forward<Us>(us)...) {}

        [[nodiscard]] decltype(auto) at_key(Key const& key) const {
            return this->self().dispatch(
                [&key](auto const& root) -> decltype(auto) { return root->at_key(key); }
            );
        }

    private:
        template <typename It>
        void seek_key_internal(Key const& key, It& it) const {
            this->self().dispatch(
                    [&key, &it](auto& root) {
                        if (!root->seek_key(it.leaf, it.iter, key, [](auto const& a, auto const& b){ return less_than(a, b); })) {
                            root->seek_end(it.leaf, it.iter);
                        }
                    }
            );
        }

    public:
        /**
         * @return an iterator pointing to the element with key 'key'
         */
        [[nodiscard]] typename Parent::iterator find(Key const& key) {
            typename Parent::iterator ret(this->self());
            seek_key_internal(key, ret);
            return ret;
        }

        /**
         * @return a const_iterator pointing to the element with key 'key'
         */
        [[nodiscard]] typename Parent::const_iterator find_const(Key const& key) const {
            typename Parent::const_iterator ret(this->self());
            seek_key_internal(key, ret);
            return ret;
        }

        /**
         * @return a const_iterator pointing to the element with key 'key'
         */
        [[nodiscard]] typename Parent::const_iterator find(Key const& key) const {
            return find_const(key);
        }

        /**
         * @return an iterator pointing to the first element with a key >= 'key'
         */
        [[nodiscard]] typename Parent::iterator lower_bound(Key const& key) {
            typename Parent::iterator ret(this->self());
            std::as_const(this->self()).dispatch([&key, &ret](auto& root) { root->seek_key(ret.leaf, ret.iter, key, [](auto const& a, auto const& b){ return less_than(a, b); }); });
            return ret;
        }

        /**
         * @return a const_iterator pointing to the first element with a key >= 'key'
         */
        [[nodiscard]] typename Parent::const_iterator lower_bound_const(Key const& key) const {
            typename Parent::const_iterator ret(this->self());
            this->self().dispatch([&key, &ret](auto& root) { root->seek_key(ret.leaf, ret.iter, key, [](auto const& a, auto const& b){ return less_than(a, b); }); });
            return ret;
        }

        /**
         * @return a const_iterator pointing to the first element with a key >= 'key'
         */
        [[nodiscard]] typename Parent::const_iterator lower_bound(Key const& key) const {
            return lower_bound_const(key);
        }

        //TODO add tests
        /**
         * @return an iterator pointing to the first element with a key >= 'key'
         */
        [[nodiscard]] typename Parent::iterator upper_bound(Key const& key) {
            typename Parent::iterator ret(this->self());
            std::as_const(this->self()).dispatch([&key, &ret](auto& root) { root->seek_key(ret.leaf, ret.iter, key, [](auto const& a, auto const& b){ return !less_than(b, a); }); });
            return ret;
        }

        //TODO add tests
        /**
         * @return a const_iterator pointing to the first element with a key > 'key'
         */
        [[nodiscard]] typename Parent::const_iterator upper_bound_const(Key const& key) const {
            typename Parent::const_iterator ret(this->self());
            this->self().dispatch([&key, &ret](auto& root) { root->seek_key(ret.leaf, ret.iter, key, [](auto const& a, auto const& b){ return !less_than(b, a); }); });
            return ret;
        }

        /**
         * @return a const_iterator pointing to the first element with a key > 'key'
         */
        [[nodiscard]] typename Parent::const_iterator upper_bound(Key const& key) const {
            return upper_bound_const(key);
        }

        /**
         * @return true if this tree contains an element with key 'key', false otherwise
         */
        [[nodiscard]] bool contains(Key const& key) const {
            return this->self().dispatch(
                [&key](auto const& root) { return root->contains(key); }
            );
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
        template <typename... Args>
        void insert_v(Args&&... args) {
            this->self().dispatch(
                Modify<InsertOrAssign<DuplicatePolicy::ignore>>(),
                [](auto const& node, Key const& key) {
                    return std::tuple<IndexType, Key const&>(node.lower_bound_index(key), key);
                },
                extractor.get_key(std::as_const(args)...),
                std::forward<Args>(args)...
            );
        }

        template <typename... Args>
        void assign_v(Args&&... args) {
            this->self().dispatch(
                Modify<Assign>(),
                [](auto const& node, Key const& key) {
                    return std::tuple<IndexType, Key const&>(node.find_key_index_checked(key), key);
                },
                extractor.get_key(std::as_const(args)...),
                std::forward<Args>(args)...
            );
        }

        void erase_key(Key const& key) {
            this->self().dispatch(
                    Modify<Erase>(),
                    [](auto const& node, Key const& key) {
                        return std::tuple<IndexType, Key const&>(node.find_key_index_checked(key), key);
                    },
                    key
            );
        }

        template <typename U>
        void update_key(Key const& key, U&& updater) {
            this->self().dispatch(
                    Modify<Update2>(),
                    [](auto const& node, Key const& key) {
                        return std::tuple<IndexType, Key const&>(node.find_key_index_checked(key), key);
                    },
                    key,
                    [&key, &updater](auto&& callback, auto const& v) {
                        Value new_val = updater(extractor.get_value(v));
                        decltype(auto) extracted = extractor.apply([](auto const&... args){ return extractor.get_key(args...); }, key, new_val);
                        if (less_than(extracted, key) || less_than(key, extracted)) { //NOLINT
                            throw std::logic_error("key from value does not match key passed to update_key!");
                        }
                        extractor.apply(callback, key, new_val);
                    }
            );
        }

        template <typename... Args>
        void insert_or_assign(Args&&... args) {
            this->self().dispatch(
                Modify<InsertOrAssign<DuplicatePolicy::replace>>(),
                [](auto const& node, Key const& key) {
                    return std::tuple<IndexType, Key const&>(node.lower_bound_index(key), key);
                },
                extractor.get_key(std::as_const(args)...),
                std::forward<Args>(args)...
            );
        }

        struct ProxyRef : public ProxyOperators<ProxyRef> {
        private:
            friend struct ProxyOperators<ProxyRef>;

            Transient& tree;
            Key const& key;

            ProxyRef(const ProxyRef& other) noexcept = default;
            ProxyRef(ProxyRef&& other) noexcept = default;
            ProxyRef& operator=(const ProxyRef& other) noexcept = default;
            ProxyRef& operator=(ProxyRef&& other) noexcept = default;

            template <typename F>
            ProxyRef& invoke_compound_assignment(F&& f) {
                tree.update_key(key, f);
                return *this;
            }

            template <typename F>
            ProxyRef& invoke_pre(F&& f) {
                tree.update_key(key, [&f](auto const& v){
                    Value copy(v);
                    f(copy);
                    return copy;
                });
                return *this;
            }

            template <typename F>
            Value invoke_post(F&& f) {
                Value ret;
                tree.update_key(key, [&f, &ret](auto const& v){
                    Value copy(v);
                    ret = f(copy);
                    return copy;
                });
                return ret;
            }
        public:
            ProxyRef(Transient& tree, Key const& key) noexcept : tree(tree), key(key) {}
            ~ProxyRef() noexcept = default;

            template <typename V>
            ProxyRef& operator=(V&& value) {
                extractor.apply(
                    [this](auto&&... args) { tree.insert_or_assign(std::forward<decltype(args)>(args)...); },
                    key, std::forward<V>(value));
                return *this;
            }

            void check_key(KeyValue const& key_value) {
                decltype(auto) extracted = extractor.get_key(key_value);
                if (less_than(extracted, key) || less_than(key, extracted)) { //NOLINT
                    throw std::logic_error("key from value does not match key passed to operator[]!");
                }
            }

            ProxyRef& operator=(KeyValue& value) {
                check_key(value);
                tree.insert_or_assign(value);
                return *this;
            }

            ProxyRef& operator=(KeyValue const& value) {
                check_key(value);
                tree.insert_or_assign(value);
                return *this;
            }

            ProxyRef& operator=(KeyValue&& value) {
                check_key(value);
                tree.insert_or_assign(std::move(value));
                return *this;
            }

            operator GetValue() const { //NOLINT
                return tree.at_key(key);
            }

            [[nodiscard]] GetValue get() const {
                return tree.at_key(key);
            }
        };

        [[nodiscard]] ProxyRef operator[](Key const& key) {
            return ProxyRef(*this, key);
        }

        [[nodiscard]] GetValue operator[](Key const& key) const {
            return this->at_key(key);
        }
    };

    template <typename Parent>
    struct Persistent : public Parent {
        template <typename... Us>
        explicit Persistent(Us&&... us) noexcept : Parent(std::forward<Us>(us)...) {}

        using SelfType = typename Parent::SelfType;

        template <typename... Args>
        [[nodiscard]] SelfType insert_v(Args&&... args) const {
            auto transient = this->self().transient();
            transient.insert_v(std::forward<Args>(args)...);
            return std::move(transient).persistent();
        }

        template <typename... Args>
        [[nodiscard]] SelfType assign_v(Args&&... args) const {
            auto transient = this->self().transient();
            transient.assign_v(std::forward<Args>(args)...);
            return std::move(transient).persistent();
        }

        template <typename... Args>
        [[nodiscard]] SelfType insert_or_assign(Args&&... args) const {
            auto transient = this->self().transient();
            transient.insert_or_assign(std::forward<Args>(args)...);
            return std::move(transient).persistent();
        }

        [[nodiscard]] SelfType erase_key(Key const& key) const {
            auto transient = this->self().transient();
            transient.erase_key(key);
            return std::move(transient).persistent();
        }

        template <typename U>
        [[nodiscard]] SelfType update_key(Key const& key, U&& updater) const {
            auto transient = this->self().transient();
            transient.update_key(key, updater);
            return std::move(transient).persistent();
        }

        [[nodiscard]] GetValue operator[](Key const& key) const {
            return this->at_key(key);
        }
    };
};

template <typename Extractor = PairExtractor<0>, typename Compare = MinComparator, typename BinarySearch = std::false_type>
struct OrderedBuilder {

    template <typename T>
    using extractor = OrderedBuilder<T, Compare, BinarySearch>;

    template <typename T>
    using compare = OrderedBuilder<Extractor, T, BinarySearch>;

    template <bool b>
    using binary_search = OrderedBuilder<Extractor, Compare, std::conditional_t<b, std::true_type, std::false_type>>;

    template <typename KeyValue>
    using build = Ordered<KeyValue, Extractor, Compare, BinarySearch>;
};
} //end namespace detail
using detail::Ordered;
using detail::OrderedBuilder;
} //end namespace bpptree
