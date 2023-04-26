//
// B++ Tree: A B+ Tree library written in C++
// Copyright (C) 2023 Jeff Plaisance
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#pragma once

#include <iostream>
#include <cstdint>
#include <array>
#include <variant>
#include <atomic>
#include <algorithm>
#include <cassert>
#include <vector>

#include "bpptree/detail/operations.hpp"
#include "bpptree/detail/sandwich.hpp"
#include "bpptree/detail/nodeptr.hpp"
#include "bpptree/detail/helpers.hpp"
#include "bpptree/detail/leafnodebase.hpp"
#include "bpptree/detail/internalnodebase.hpp"
#include "bpptree/detail/iterator.hpp"
#include "bpptree/detail/nodetypes.hpp"
#include "bpptree/detail/modify.hpp"

namespace bpptree {
namespace detail {

/**
 * A B++ tree is a B+ tree with a pluggable set of mixins
 * @tparam Value The value type of the B++ Tree
 * @tparam LeafNodeBytes The maximum number of bytes in a leaf node
 * @tparam InternalNodeBytes The maximum number of bytes in an internal node
 * @tparam depthLimit The maximum depth of the tree. Setting this lower reduces code size and compile times
 * @tparam Ts The mixins to use with this B++ tree
 */
template <
    typename Value,
    int LeafNodeBytes = 512,
    int InternalNodeBytes = 512,
    int depthLimit = 16,
    typename... Ts>
class BppTreeDetail {
private:

    using NodeTypes = NodeTypesDetail<Value, LeafNodeBytes, InternalNodeBytes, depthLimit, Ts...>;

    static constexpr int leaf_node_size = NodeTypes::leaf_node_size;

    static constexpr int internal_node_size = NodeTypes::internal_node_size;

    static constexpr int maxDepth = NodeTypes::maxDepth;

    static constexpr size_t maxSize = NodeTypes::maxSize;

    using LeafNode = typename NodeTypes::template LeafNode<leaf_node_size>;

    template <int depth>
    using InternalNode = typename NodeTypes::template InternalNode<leaf_node_size, internal_node_size, depth>;

    using RootType = typename NodeTypes::RootType;

public:
    template <typename Parent>
    struct Shared : public Parent {
    protected:
        RootType rootVariant;
        size_t treeSize;
        uint64_t mod_count = 0;

        Shared() noexcept : Parent(), rootVariant(makePtr<LeafNode>()), treeSize(0) {}

        Shared(RootType const& rootVariant, size_t size) noexcept : Parent(), rootVariant(rootVariant), treeSize(size) {}

        Shared(RootType&& rootVariant, size_t size) noexcept : Parent(), rootVariant(std::move(rootVariant)), treeSize(size) {}

        using Modifiers = ModifyTypes<LeafNode, InternalNode, maxDepth>;

        friend Modifiers;

        template <typename Operation>
        using Modify = typename Modifiers::template Modify<Operation>;

        template <typename Operation, typename... Us>
        decltype(auto) dispatch(Operation&& op, Us&&... us) {
            return std::visit([this, &op, &us...](auto& root) -> decltype(auto) { return op(this->self(), root, std::forward<Us>(us)...); }, rootVariant);
        }

        template <typename Operation, typename... Us>
        [[nodiscard]] decltype(auto) dispatch(Operation const& op, Us const&... us) const {
            return std::visit([&op, &us...](auto const& root) -> decltype(auto) { return op(root, us...); }, rootVariant);
        }

        template <bool isConst, bool reverse>
        using IteratorType = IteratorDetail<Value, typename Parent::SelfType, LeafNode, isConst, reverse>;

    public:
        [[nodiscard]] size_t size() {
            return treeSize;
        }

        [[nodiscard]] constexpr size_t max_size() {
            return maxSize;
        }

        [[nodiscard]] size_t depth() {
            return std::as_const(this->self()).dispatch([](auto const& root){ return static_cast<size_t>(root->depth); });
        }

        [[nodiscard]] constexpr size_t max_depth() {
            return maxDepth;
        }

        using iterator = IteratorType<false, false>;

        using const_iterator = IteratorType<true, false>;

        using reverse_iterator = IteratorType<false, true>;

        using const_reverse_iterator = IteratorType<true, true>;
    private:
        friend iterator;
        friend const_iterator;
        friend reverse_iterator;
        friend const_reverse_iterator;
    public:
        [[nodiscard]] iterator begin() {
            iterator ret(this->self());
            std::as_const(this->self()).dispatch([&ret](auto const& root) { root->seekBegin(ret.leaf, ret.iter); });
            return ret;
        }

        [[nodiscard]] iterator end() {
            iterator ret(this->self());
            std::as_const(this->self()).dispatch([&ret](auto const& root) { root->seekEnd(ret.leaf, ret.iter); });
            return ret;
        }

        [[nodiscard]] const_iterator cbegin() const {
            const_iterator ret(this->self());
            this->self().dispatch([&ret](auto const& root) { root->seekBegin(ret.leaf, ret.iter); });
            return ret;
        }

        [[nodiscard]] const_iterator cend() const {
            const_iterator ret(this->self());
            this->self().dispatch([&ret](auto const& root) { root->seekEnd(ret.leaf, ret.iter); });
            return ret;
        }

        [[nodiscard]] const_iterator begin() const {
            return cbegin();
        }

        [[nodiscard]] const_iterator end() const {
            return cend();
        }

        [[nodiscard]] reverse_iterator rbegin() {
            reverse_iterator ret(this->self());
            std::as_const(this->self()).dispatch([&ret](auto const& root) { root->seekEnd(ret.leaf, ret.iter); });
            ++ret;
            return ret;
        }

        [[nodiscard]] reverse_iterator rend() {
            reverse_iterator ret(this->self());
            ret.leaf = nullptr;
            ret.iter = reverse_iterator::rend;
            return ret;
        }

        [[nodiscard]] const_reverse_iterator crbegin() const {
            const_reverse_iterator ret(this->self());
            this->self().dispatch([&ret](auto const& root) { root->seekEnd(ret.leaf, ret.iter); });
            ++ret;
            return ret;
        }

        [[nodiscard]] const_reverse_iterator crend() const {
            const_reverse_iterator ret(this->self());
            ret.leaf = nullptr;
            ret.iter = const_reverse_iterator::rend;
            return ret;
        }

        [[nodiscard]] const_reverse_iterator rbegin() const {
            return crbegin();
        }

        [[nodiscard]] const_reverse_iterator rend() const {
            return crend();
        }

        [[nodiscard]] Value const& front() const {
            return this->self().dispatch([](auto const& root) -> decltype(auto) { return root->front(); });
        }

        [[nodiscard]] Value const& back() const {
            return this->self().dispatch([](auto const& root) -> decltype(auto) { return root->back(); });
        }

        [[nodiscard]] bool empty() const {
            return treeSize == 0;
        }
    };
private:

    template <typename Parent>
    using TransientMixin = typename Mixin<
        Ts::template Transient...,
        Ts::template Shared...,
        Shared
    >::template chain_t<Parent>;

    template <typename Parent>
    using PersistentMixin = typename Mixin<
        Ts::template Persistent...,
        Ts::template Shared...,
        Shared
    >::template chain_t<Parent>;

public:
    struct Persistent;

    struct Transient : public TransientMixin<Transient> {

        using Parent = TransientMixin<Transient>;

        template <typename... Us>
        explicit Transient(Us&&... us) noexcept : Parent(std::forward<Us>(us)...) {}

        [[nodiscard]] Persistent persistent() & {
            return Persistent(this->rootVariant, this->treeSize);
        }

        [[nodiscard]] Persistent persistent() && {
            return Persistent(std::move(this->rootVariant), this->treeSize);
        }

    private:
        friend Persistent;

        template <typename... Args>
        uint64_t insert2(uint64_t const& it, Args&&... args) {
            return this->self().dispatch(Modify<Insert>(), find_iterator, it, std::forward<Args>(args)...);
        }

        uint64_t erase2(uint64_t const& it) {
            return this->self().dispatch(Modify<Erase>(), find_iterator, it);
        }

    protected:
        template<typename T>
        using Modify = typename Parent::template Modify<T>;

    public:
        template <typename It, typename... Args>
        void assign(It const& it, Args&&... args) {
            this->self().dispatch(Modify<Assign>(), find_iterator, it.iter, std::forward<Args>(args)...);
        }

        template <typename It, typename... Args>
        void insert(It&& it, Args&&... args) {
            if constexpr (std::decay_t<It>::is_reversed) {
                --it;
            }
            it.iter = insert2(it.iter, std::forward<Args>(args)...);
        }

        template <typename It>
        void erase(It&& it) {
            it.iter = erase2(it.iter);
            if constexpr (std::is_reference_v<It> && std::decay_t<It>::is_reversed) {
                ++it;
            }
        }

        template <typename It, typename U>
        void update(It const& it, U&& updater) {
            this->self().dispatch(Modify<Update>(), find_iterator, it.iter, updater);
        }

        template <typename... Args>
        void emplace_front(Args&&... args) {
            this->self().dispatch(Modify<Insert>(), find_first, Empty::empty, std::forward<Args>(args)...);
        }

        template <typename... Args>
        void emplace_back(Args&&... args) {
            this->self().dispatch(Modify<Insert>(), find_last, Empty::empty, std::forward<Args>(args)...);
        }

        template <typename... Args>
        void push_front(Args&&... args) {
            emplace_front(std::forward<Args>(args)...);
        }

        template <typename... Args>
        void push_back(Args&&... args) {
            emplace_back(std::forward<Args>(args)...);
        }

        Value pop_front() {
            Value ret = this->self().front();
            this->self().dispatch(Modify<Erase>(), find_first, Empty::empty);
            return ret;
        }

        Value pop_back() {
            Value ret = this->self().back();
            this->self().dispatch(Modify<Erase>(), find_last, Empty::empty);
            return ret;
        }

        void clear() {
            this->self().rootVariant = makePtr<LeafNode>();
            this->treeSize = 0;
            this->mod_count++;
        }
    };

    struct Persistent : public PersistentMixin<Persistent> {

        using Parent = PersistentMixin<Persistent>;

        template <typename... Us>
        explicit Persistent(Us&&... us) noexcept : Parent(std::forward<Us>(us)...) {
            this->self().dispatch([](auto&, auto& root) {
                root->makePersistent();
            });
        }

        [[nodiscard]] Transient transient() const& {
            return Transient(this->rootVariant, this->treeSize);
        }

        [[nodiscard]] Transient transient() && {
            return Transient(std::move(this->rootVariant), this->treeSize);
        }

        template <typename It, typename... Args>
        [[nodiscard]] Persistent assign(It const& it, Args&&... args) const {
            auto transient = this->self().transient();
            transient.assign(it, std::forward<Args>(args)...);
            return std::move(transient).persistent();
        }

        template <typename It, typename... Args>
        [[nodiscard]] Persistent insert(It const& it, Args&&... args) const {
            auto transient = this->self().transient();
            transient.insert2(it.iter, std::forward<Args>(args)...);
            return std::move(transient).persistent();
        }

        template <typename It>
        [[nodiscard]] Persistent erase(It const& it) const {
            auto transient = this->self().transient();
            transient.erase2(it.iter);
            return std::move(transient).persistent();
        }

        template <typename It, typename U>
        [[nodiscard]] Persistent update(It const& it, U&& updater) const {
            auto transient = this->self().transient();
            transient.update(it, updater);
            return std::move(transient).persistent();
        }

        template <typename... Args>
        [[nodiscard]] Persistent emplace_front(Args&&... args) const {
            auto transient = this->self().transient();
            transient.emplace_front(std::forward<Args>(args)...);
            return std::move(transient).persistent();
        }

        template <typename... Args>
        [[nodiscard]] Persistent emplace_back(Args&&... args) const {
            auto transient = this->self().transient();
            transient.emplace_back(std::forward<Args>(args)...);
            return std::move(transient).persistent();
        }

        template <typename... Args>
        [[nodiscard]] Persistent push_front(Args&&... args) const {
            return emplace_front(std::forward<Args>(args)...);
        }

        template <typename... Args>
        [[nodiscard]] Persistent push_back(Args&&... args) const {
            return emplace_back(std::forward<Args>(args)...);
        }

        [[nodiscard]] Persistent pop_front() const {
            auto transient = this->self().transient();
            transient.pop_front();
            return std::move(transient).persistent();
        }

        [[nodiscard]] Persistent pop_back() const {
            auto transient = this->self().transient();
            transient.pop_back();
            return std::move(transient).persistent();
        }
    };
};

template <typename Value, int LeafNodeBytes = 512, int InternalNodeBytes = 512, int depthLimit = 16>
struct BppTree {
    template <typename... Args>
    using mixins = BppTreeDetail<Value, LeafNodeBytes, InternalNodeBytes, depthLimit, typename Args::template build<Value>...>;

    template <template<typename...> typename... Ts>
    using mixins2 = BppTreeDetail<Value, LeafNodeBytes, InternalNodeBytes, depthLimit, Ts<Value>...>;

    template <typename... Ts>
    struct mixins3 {
        template <template<typename...> typename... Us>
        using mixins = BppTreeDetail<Value, LeafNodeBytes, InternalNodeBytes, depthLimit, Ts..., Us<Value>...>;
    };
};
} //end namespace detail
using detail::BppTree;
} //end namespace bpptree
