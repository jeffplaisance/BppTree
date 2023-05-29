//
// B++ Tree: A B+ Tree library written in C++
// Copyright (C) 2023 Jeff Plaisance
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#pragma once

#include <atomic>

namespace bpptree::detail {

template <
        typename Value,
        ssize leaf_node_bytes = 512,
        ssize internal_node_bytes = 512,
        ssize depth_limit = 16,
        bool disable_exceptions = true,
        typename... Ts>
struct NodeTypesDetail {
    template <typename Parent>
    struct NodeBase;

    template <typename Parent>
    struct NodeInfoBase : public Parent {
        NodeInfoBase() = default;

        template <typename P>
        explicit constexpr NodeInfoBase(P const&, bool const) {}
    };

    template <typename Derived>
    using NodeInfoMixin = Chain<Derived, Ts::template NodeInfo..., NodeInfoBase>;

    template <typename PtrType>
    struct NodeInfo : public NodeInfoMixin<NodeInfo<PtrType>> {
        using Parent = NodeInfoMixin<NodeInfo<PtrType>>;

        NodePtr<PtrType> ptr{};
        bool ptr_changed = false;

        NodeInfo() = default;

        template <typename P>
        NodeInfo(P&& p, const bool changed) noexcept(disable_exceptions) : Parent(p, changed), ptr(), ptr_changed(changed) {
            if (changed) {
                this->ptr = std::forward<P>(p);
            }
        }
    };

    template <typename Derived, auto leaf_size>
    struct LeafNodeMixin {
        template <typename Parent>
        using LeafNodeBaseCurried = LeafNodeBase<Parent, Value, leaf_size, disable_exceptions>;
        using Type = Chain<Derived,
                Ts::template LeafNode...,
                LeafNodeBaseCurried,
                NodeBase>;
    };

    template<typename Derived, auto leaf_size, auto internal_size, auto depth>
    struct InternalNodeMixin {
        template <typename Parent>
        using InternalNodeBaseCurried = InternalNodeBase<Parent, Value, leaf_size, internal_size, depth, disable_exceptions>;
        using Type = Chain<Derived,
                Curry<Ts::template InternalNode, internal_size>::template Mixin...,
                InternalNodeBaseCurried,
                NodeBase>;
    };

    template <auto leaf_size>
    struct LeafNode : public LeafNodeMixin<LeafNode<leaf_size>, leaf_size>::Type {};

    template<auto leaf_size, auto internal_size, auto depth>
    struct InternalNode : public InternalNodeMixin<InternalNode<leaf_size, internal_size, depth>, leaf_size, internal_size, depth>::Type {};

    template <typename Parent>
    struct NodeBase : public Parent {
        template <typename PtrType>
        using NodeInfoType = NodeInfo<PtrType>;

        template <auto leaf_size>
        using LeafNodeType = LeafNode<leaf_size>;

        template <auto leaf_size, auto internal_size, auto depth>
        using InternalNodeType = InternalNode<leaf_size, internal_size, depth>;

        std::atomic<uint32_t> ref_count = 1;
        uint16_t length = 0;
        bool persistent = false;

        NodeBase() = default;

        // ref_count and persistent are intentionally not copied
        NodeBase(NodeBase const& other) noexcept : Parent(other), length(other.length) {}

        // copy assignment is deleted because there is no scenario in which overwriting an existing node makes sense
        NodeBase& operator=(NodeBase const& other) = delete;

        ~NodeBase() = default;
    };

    template<int leaf_size>
    static constexpr int get_leaf_node_size3() {
        if constexpr (sizeof(LeafNode<leaf_size>) <= leaf_node_bytes) {
            return leaf_size;
        } else {
            return get_leaf_node_size3<leaf_size - 1>();
        }
    }

    template<int leaf_size>
    static constexpr int get_leaf_node_size2() {
        constexpr ssize size = sizeof(LeafNode<leaf_size>);
        if constexpr (size > leaf_node_bytes) {
            return get_leaf_node_size3<leaf_size - 1>();
        } else if constexpr (size + sizeof(Value) > leaf_node_bytes) {
            return leaf_size;
        } else {
            return get_leaf_node_size2<leaf_size + 1>();
        }
    }

    static constexpr int get_leaf_node_size() {
        constexpr int initial = (leaf_node_bytes - 8) / sizeof(Value);
        return get_leaf_node_size2<initial>();
    }

    static constexpr int leaf_node_size = get_leaf_node_size();
    static_assert(leaf_node_size > 0);
    static_assert(leaf_node_size < 65536);

    // each element added to an internal node increases its size by
    // at least this amount but potentially more due to padding
    static constexpr ssize internal_element_size_lower_bound = (Ts::sizeof_hint() + ... + sizeof(void*));

    template<int internal_size>
    static constexpr int get_internal_node_size3() {
        if constexpr (sizeof(InternalNode<leaf_node_size, internal_size, 2>) <= internal_node_bytes) {
            return internal_size;
        } else {
            return get_internal_node_size3<internal_size - 1>();
        }
    }

    template<int internal_size>
    static constexpr int get_internal_node_size2() {
        constexpr ssize size = sizeof(InternalNode<leaf_node_size, internal_size, 2>);
        if constexpr (size > internal_node_bytes) {
            return get_internal_node_size3<internal_size - 1>();
        } else if constexpr (size + internal_element_size_lower_bound > internal_node_bytes) {
            return internal_size;
        } else {
            return get_internal_node_size2<internal_size + 1>();
        }
    }

    static constexpr int get_internal_node_size() {
        constexpr int initial = (internal_node_bytes - 8) / internal_element_size_lower_bound;
        return get_internal_node_size2<initial>();
    }

    static constexpr int internal_node_size = get_internal_node_size();
    static_assert(internal_node_size >= 4);
    static_assert(internal_node_size < 65536);

    template <size_t size, int depth>
    static constexpr int find_max_depth() {
        if constexpr (size > 64) {
            return depth - 1;
        } else if constexpr (depth == depth_limit) {
            return depth;
        } else {
            return find_max_depth<size + bits_required<internal_node_size>(), depth + 1>();
        }
    }

    template <size_t a, size_t b>
    static constexpr size_t pow() {
        if constexpr (b == 0) {
            return 1;
        } else {
            return a * pow<a, b - 1>();
        }
    }

    static constexpr int max_depth = find_max_depth<bits_required<leaf_node_size>(), 1>();

    static constexpr size_t max_size = leaf_node_size * pow<internal_node_size, max_depth - 1>();

    template <int head, int... tail>
    struct RootVariant {
        using type = typename RootVariant<head - 1, head, tail...>::type;
    };

    template <int... args>
    struct RootVariant<1, args...> {
        using type = std::variant<NodePtr<LeafNode<leaf_node_size>>, NodePtr<InternalNode<leaf_node_size, internal_node_size, args>>...>;
    };

    using RootType = typename RootVariant<max_depth>::type;
};
} //end namespace bpptree::detail
