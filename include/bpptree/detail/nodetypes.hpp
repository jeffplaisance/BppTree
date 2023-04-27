//
// B++ Tree: A B+ Tree library written in C++
// Copyright (C) 2023 Jeff Plaisance
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#pragma once

namespace bpptree::detail {

template <
        typename Value,
        ssize leaf_node_bytes = 512,
        ssize internal_node_bytes = 512,
        ssize depth_limit = 16,
        typename... Ts>
struct NodeTypesDetail {
    template <typename Parent>
    struct Params;

    template <typename Parent>
    struct NodeInfoBase : public Parent {
        NodeInfoBase() noexcept = default;

        template <typename P>
        explicit constexpr NodeInfoBase(P const&, bool const) noexcept {}
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
        NodeInfo(P&& p, const bool changed) : Parent(p, changed), ptr(), ptr_changed(changed) {
            if (changed) {
                this->ptr = std::forward<P>(p);
            }
        }
    };

    template <typename Derived, auto leaf_size>
    struct LeafNodeMixin {
        template <typename Parent>
        using LeafNodeBaseCurried = LeafNodeBase<Parent, Value, leaf_size>;
        using Type = Chain<Derived,
                Ts::template LeafNode...,
                LeafNodeBaseCurried,
                Params>;
    };

    template<typename Derived, auto leaf_size, auto internal_size, auto depth>
    struct InternalNodeMixin {
        template <typename Parent>
        using InternalNodeBaseCurried = InternalNodeBase<Parent, Value, leaf_size, internal_size, depth>;
        using Type = Chain<Derived,
                Curry<Ts::template InternalNode, internal_size>::template Mixin...,
                InternalNodeBaseCurried,
                Params>;
    };

    #if defined(_MSC_VER) \
    // Disables "structure was padded due to alignment specifier" warnings
    #pragma warning(push)
    #pragma warning(disable : 4324)
    #endif

    template <auto leaf_size>
    struct alignas(hardware_constructive_interference_size) LeafNode : public LeafNodeMixin<LeafNode<leaf_size>, leaf_size>::Type {

        using Parent = typename LeafNodeMixin<LeafNode<leaf_size>, leaf_size>::Type;

        std::atomic<uint32_t> ref_count = 1;

        LeafNode() noexcept : Parent() {};

        template <typename T>
        LeafNode(T&& t, bool persistent) noexcept : Parent(std::forward<T>(t)) {
            this->persistent = persistent;
        }
    };

    template<auto leaf_size, auto internal_size, auto depth>
    struct alignas(hardware_constructive_interference_size) InternalNode : public InternalNodeMixin<InternalNode<leaf_size, internal_size, depth>, leaf_size, internal_size, depth>::Type {

        using Parent = typename InternalNodeMixin<InternalNode<leaf_size, internal_size, depth>, leaf_size, internal_size, depth>::Type;

        std::atomic<uint32_t> ref_count = 1;

        InternalNode() noexcept : Parent() {};

        template <typename T>
        InternalNode(T&& t, bool persistent) noexcept : Parent(std::forward<T>(t)) {
            this->persistent = persistent;
        }
    };

    #if defined(_MSC_VER)
    #pragma warning(pop)
    #endif

    template <typename Parent>
    struct Params : public Parent {
        template <typename PtrType>
        using NodeInfoType = NodeInfo<PtrType>;

        template <auto leaf_size>
        using LeafNodeType = LeafNode<leaf_size>;

        template <auto leaf_size, auto internal_size, auto depth>
        using InternalNodeType = InternalNode<leaf_size, internal_size, depth>;
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
        if constexpr (sizeof(LeafNode<leaf_size>) > leaf_node_bytes) {
            return get_leaf_node_size3<leaf_size - 1>();
        } else {
            return get_leaf_node_size2<leaf_size + 1>();
        }
    }

    static constexpr int get_leaf_node_size() {
        constexpr int initial = (leaf_node_bytes-8)/sizeof(Value);
        return get_leaf_node_size2<initial>();
    }

    static constexpr int leaf_node_size = get_leaf_node_size();
    static_assert(leaf_node_size > 0);
    static_assert(leaf_node_size < 65536);

    template<int internal_size>
    static constexpr int get_internal_node_size3() {
        if constexpr (sizeof(InternalNode<leaf_node_size, internal_size, 3>) <= internal_node_bytes) {
            return internal_size;
        } else {
            return get_internal_node_size3<internal_size - 1>();
        }
    }

    template<int internal_size>
    static constexpr int get_internal_node_size2() {
        if constexpr (sizeof(InternalNode<leaf_node_size, internal_size, 3>) > internal_node_bytes) {
            return get_internal_node_size3<internal_size - 1>();
        } else {
            return get_internal_node_size2<internal_size + 8>();
        }
    }

    static constexpr int get_internal_node_size() {
        return get_internal_node_size2<1>();
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

    template <int U, int... Us>
    struct RootVariant {
        using type = typename RootVariant<U - 1, U, Us...>::type;
    };

    template <int... Us>
    struct RootVariant<1, Us...> {
        using type = std::variant<NodePtr<LeafNode<leaf_node_size>>, NodePtr<InternalNode<leaf_node_size, internal_node_size, Us>>...>;
    };

    using RootType = typename RootVariant<max_depth>::type;
};
} //end namespace bpptree::detail
