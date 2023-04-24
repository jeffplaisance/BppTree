#pragma once

namespace bpptree::detail {

template <
        typename Value,
        int LeafNodeBytes = 512,
        int InternalNodeBytes = 512,
        int depthLimit = 16,
        typename... Ts>
struct NodeTypes {
    template <typename Parent>
    struct Params;

    template <typename PtrType>
    using NodeInfo =
        MixinT<Ts::template NodeInfo...,
               curry<NodeInfoBase, PtrType>::template mixin>;

    template <typename Parent, auto LeafSize>
    struct LeafNodeMixin {
        template <typename P>
        using LeafNodeBaseCurried = LeafNodeBase<P, Value, LeafSize>;
        using Type = chain_t<Parent,
                Ts::template LeafNode...,
                LeafNodeBaseCurried,
                Params>;
    };

    template<typename Parent, auto LeafSize, auto InternalSize, auto Depth>
    struct InternalNodeMixin {
        template <typename P>
        using InternalNodeBaseCurried = InternalNodeBase<P, Value, LeafSize, InternalSize, Depth>;
        using Type = chain_t<Parent,
                curry2<Ts::template InternalNode, InternalSize>::template mixin...,
                InternalNodeBaseCurried,
                Params>;
    };

    #if defined(_MSC_VER) \
    // Disables "structure was padded due to alignment specifier" warnings
    #pragma warning(push)
    #pragma warning(disable : 4324)
    #endif

    template <auto LeafSize>
    struct alignas(BPPTREE_CACHE_ALIGN) LeafNode : public LeafNodeMixin<LeafNode<LeafSize>, LeafSize>::Type {

        using Parent = typename LeafNodeMixin<LeafNode<LeafSize>, LeafSize>::Type;

        std::atomic<uint32_t> refCount = 1;

        LeafNode() noexcept : Parent() {};

        template <typename T>
        LeafNode(T&& t, bool persistent) noexcept : Parent(std::forward<T>(t)) {
            this->persistent = persistent;
        }
    };

    template<auto LeafSize, auto InternalSize, auto Depth>
    struct alignas(BPPTREE_CACHE_ALIGN) InternalNode : public InternalNodeMixin<InternalNode<LeafSize, InternalSize, Depth>, LeafSize, InternalSize, Depth>::Type {

        using Parent = typename InternalNodeMixin<InternalNode<LeafSize, InternalSize, Depth>, LeafSize, InternalSize, Depth>::Type;

        std::atomic<uint32_t> refCount = 1;

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

        template <auto LeafSize>
        using LeafNodeType = LeafNode<LeafSize>;

        template <auto LeafSize, auto InternalSize, auto Depth>
        using InternalNodeType = InternalNode<LeafSize, InternalSize, Depth>;
    };

    template<int LeafSize>
    static constexpr int getLeafNodeSize3() {
        if constexpr (sizeof(LeafNode<LeafSize>) <= LeafNodeBytes) {
            return LeafSize;
        } else {
            return getLeafNodeSize3<LeafSize - 1>();
        }
    }

    template<int LeafSize>
    static constexpr int getLeafNodeSize2() {
        if constexpr (sizeof(LeafNode<LeafSize>) > LeafNodeBytes) {
            return getLeafNodeSize3<LeafSize - 1>();
        } else {
            return getLeafNodeSize2<LeafSize + 1>();
        }
    }

    static constexpr int getLeafNodeSize() {
        constexpr int initial = (LeafNodeBytes-8)/sizeof(Value);
        return getLeafNodeSize2<initial>();
    }

    static constexpr int leaf_node_size = getLeafNodeSize();
    static_assert(leaf_node_size > 0);
    static_assert(leaf_node_size < 65536);

    template<int InternalSize>
    static constexpr int getInternalNodeSize3() {
        if constexpr (sizeof(InternalNode<leaf_node_size, InternalSize, 3>) <= InternalNodeBytes) {
            return InternalSize;
        } else {
            return getInternalNodeSize3<InternalSize - 1>();
        }
    }

    template<int InternalSize>
    static constexpr int getInternalNodeSize2() {
        if constexpr (sizeof(InternalNode<leaf_node_size, InternalSize, 3>) > InternalNodeBytes) {
            return getInternalNodeSize3<InternalSize - 1>();
        } else {
            return getInternalNodeSize2<InternalSize + 8>();
        }
    }

    static constexpr int getInternalNodeSize() {
        return getInternalNodeSize2<1>();
    }

    static constexpr int internal_node_size = getInternalNodeSize();
    static_assert(internal_node_size >= 4);
    static_assert(internal_node_size < 65536);

    template <size_t Size, int Depth>
    static constexpr int findMaxDepth() {
        if constexpr (Size > 64) {
            return Depth - 1;
        } else if constexpr (Depth == depthLimit) {
            return Depth;
        } else {
            return findMaxDepth<Size + bits_required<internal_node_size>(), Depth + 1>();
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

    static constexpr int maxDepth = findMaxDepth<bits_required<leaf_node_size>(), 1>();

    static constexpr size_t maxSize = leaf_node_size * pow<internal_node_size, maxDepth - 1>();

    template <int U, int... Us>
    struct RootVariant {
        using type = typename RootVariant<U - 1, U, Us...>::type;
    };

    template <int... Us>
    struct RootVariant<1, Us...> {
        using type = std::variant<NodePtr<LeafNode<leaf_node_size>>, NodePtr<InternalNode<leaf_node_size, internal_node_size, Us>>...>;
    };

    using RootType = typename RootVariant<maxDepth>::type;
};
} //end namespace bpptree::detail
