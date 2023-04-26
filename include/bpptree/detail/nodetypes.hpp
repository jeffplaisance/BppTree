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

    template <typename PtrType>
    using NodeInfo =
        MixinT<Ts::template NodeInfo...,
               curry<NodeInfoBase, PtrType>::template mixin>;

    template <typename Parent, auto leaf_size>
    struct LeafNodeMixin {
        template <typename P>
        using LeafNodeBaseCurried = LeafNodeBase<P, Value, leaf_size>;
        using Type = chain_t<Parent,
                Ts::template LeafNode...,
                LeafNodeBaseCurried,
                Params>;
    };

    template<typename Parent, auto leaf_size, auto internal_size, auto depth>
    struct InternalNodeMixin {
        template <typename P>
        using InternalNodeBaseCurried = InternalNodeBase<P, Value, leaf_size, internal_size, depth>;
        using Type = chain_t<Parent,
                curry2<Ts::template InternalNode, internal_size>::template mixin...,
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
    static constexpr int getLeafNodeSize3() {
        if constexpr (sizeof(LeafNode<leaf_size>) <= leaf_node_bytes) {
            return leaf_size;
        } else {
            return getLeafNodeSize3<leaf_size - 1>();
        }
    }

    template<int leaf_size>
    static constexpr int getLeafNodeSize2() {
        if constexpr (sizeof(LeafNode<leaf_size>) > leaf_node_bytes) {
            return getLeafNodeSize3<leaf_size - 1>();
        } else {
            return getLeafNodeSize2<leaf_size + 1>();
        }
    }

    static constexpr int getLeafNodeSize() {
        constexpr int initial = (leaf_node_bytes-8)/sizeof(Value);
        return getLeafNodeSize2<initial>();
    }

    static constexpr int leaf_node_size = getLeafNodeSize();
    static_assert(leaf_node_size > 0);
    static_assert(leaf_node_size < 65536);

    template<int internal_size>
    static constexpr int getInternalNodeSize3() {
        if constexpr (sizeof(InternalNode<leaf_node_size, internal_size, 3>) <= internal_node_bytes) {
            return internal_size;
        } else {
            return getInternalNodeSize3<internal_size - 1>();
        }
    }

    template<int internal_size>
    static constexpr int getInternalNodeSize2() {
        if constexpr (sizeof(InternalNode<leaf_node_size, internal_size, 3>) > internal_node_bytes) {
            return getInternalNodeSize3<internal_size - 1>();
        } else {
            return getInternalNodeSize2<internal_size + 8>();
        }
    }

    static constexpr int getInternalNodeSize() {
        return getInternalNodeSize2<1>();
    }

    static constexpr int internal_node_size = getInternalNodeSize();
    static_assert(internal_node_size >= 4);
    static_assert(internal_node_size < 65536);

    template <size_t size, int depth>
    static constexpr int findMaxDepth() {
        if constexpr (size > 64) {
            return depth - 1;
        } else if constexpr (depth == depth_limit) {
            return depth;
        } else {
            return findMaxDepth<size + bits_required<internal_node_size>(), depth + 1>();
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

    static constexpr int max_depth = findMaxDepth<bits_required<leaf_node_size>(), 1>();

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
