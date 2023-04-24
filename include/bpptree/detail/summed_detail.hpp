#pragma once

#include <tuple>
#include <cstdint>
#include "helpers.hpp"

namespace bpptree::detail {

template <typename Parent, typename SumType, typename Extractor>
struct SummedLeafNode : public Parent {

    static constexpr Extractor extractor{};

    using InfoType = typename Parent::InfoType;

    auto findIndexForward(SumType target) const {
        std::tuple<IndexType, SumType> ret(0, target);
        auto& [index, remainder] = ret;
        while (index < this->length) {
            SumType m = extractor(this->values[index]);
            if (m < remainder) {
                remainder -= m;
                ++index;
            } else {
                break;
            }
        }
        return ret;
    }

    template <typename I>
    void seekIndexForward(
            I& it,
            SumType target
    ) const {
        auto [index, remainder] = findIndexForward(target);
        this->setIndex(it.iter, index);
        it.leaf = &this->self();
    }

    [[nodiscard]] SumType sum() const {
        SumType sum{};
        for (IndexType i = 0; i < this->length; i++) {
            sum += extractor(this->values[i]);
        }
        return sum;
    }

    void sum_inclusive(uint64_t it, SumType& sum) const {
        IndexType index = this->getIndex(it);
        for (IndexType i = 0; i <= index; ++i) {
            sum += extractor(this->values[i]);
        }
    }

    void sum_exclusive(uint64_t it, SumType& sum) const {
        IndexType index = this->getIndex(it);
        for (IndexType i = 0; i < index; ++i) {
            sum += extractor(this->values[i]);
        }
    }

    template <typename... Args>
    void computeDeltaInsert2(IndexType index, InfoType& nodeInfo, Args const&... args) const {
        nodeInfo.sum = extractor(args...);
        Parent::computeDeltaInsert2(index, nodeInfo, args...);
    }

    template <typename... Args>
    void computeDeltaSet2(IndexType index, InfoType& nodeInfo, Args const&... args) const {
        nodeInfo.sum = extractor(args...) - extractor(this->values[index]);
        Parent::computeDeltaSet2(index, nodeInfo, args...);
    }

    void computeDeltaErase2(IndexType index, InfoType& nodeInfo) const {
        if constexpr (std::is_unsigned_v<SumType>) {
            nodeInfo.sum = ~extractor(this->values[index]) + 1;
        } else {
            nodeInfo.sum = -extractor(this->values[index]);
        }
        Parent::computeDeltaErase2(index, nodeInfo);
    }
};

template <typename Parent, typename SumType, auto internal_size>
struct SummedInternalNode : public Parent {

    using NodeType = typename Parent::NodeType;

    using ChildType = typename Parent::ChildType;

    template <typename PtrType>
    using InfoType = typename Parent::template InfoType<PtrType>;

    template <typename PtrType>
    using SplitType = typename Parent::template SplitType<PtrType>;

    safe_array<SumType, internal_size> child_sums{};

    void moveElement2(IndexType destIndex, NodeType& source, IndexType sourceIndex) {
        child_sums[destIndex] = std::move(source.child_sums[sourceIndex]);
        Parent::moveElement2(destIndex, source, sourceIndex);
    }

    void copyElement2(IndexType destIndex, NodeType const& source, IndexType sourceIndex) {
        child_sums[destIndex] = source.child_sums[sourceIndex];
        Parent::copyElement2(destIndex, source, sourceIndex);
    }

    void replaceElement2(IndexType index, InfoType<ChildType>& t) {
        child_sums[index] += t.sum;
        Parent::replaceElement2(index, t);
    }

    void setElement2(IndexType index, InfoType<ChildType>& t) {
        child_sums[index] = std::move(t.sum);
        Parent::setElement2(index, t);
    }

    void computeDeltaSplit2(SplitType<ChildType> const& split, InfoType<NodeType>& nodeInfo, IndexType index) const {
        nodeInfo.sum = split.left.sum + split.right.sum - child_sums[index];
        Parent::computeDeltaSplit2(split, nodeInfo, index);
    }

    void computeDeltaReplace2(InfoType<ChildType> const& update, InfoType<NodeType>& nodeInfo, IndexType index) const {
        nodeInfo.sum = update.sum;
        Parent::computeDeltaReplace2(update, nodeInfo, index);
    }

    void computeDeltaErase2(IndexType index, InfoType<NodeType>& nodeInfo) const {
        if constexpr (std::is_unsigned_v<SumType>) {
            nodeInfo.sum = ~child_sums[index] + 1;
        } else {
            nodeInfo.sum = -child_sums[index];
        }
        Parent::computeDeltaErase2(index, nodeInfo);
    }

    auto findIndexForward(SumType target) const {
        std::tuple<IndexType, SumType> ret(0, target);
        auto& [index, remainder] = ret;
        while (index < this->length - 1) {
            if (child_sums[index] < remainder) {
                remainder -= child_sums[index];
                ++index;
            } else {
                break;
            }
        }
        return ret;
    }

    template <typename I>
    void seekIndexForward(
            I& it,
            SumType target
    ) const {
        auto [index, remainder] = findIndexForward(target);
        this->setIndex(it.iter, index);
        this->pointers[this->getIndex(it.iter)]->seekIndexForward(it, remainder);
    }

    [[nodiscard]] SumType sum() const {
        SumType ret{};
        for (IndexType i = 0; i < this->length; ++i) {
            ret += child_sums[i];
        }
        return ret;
    }

    void sum_inclusive(uint64_t it, SumType& sum) const {
        IndexType index = this->getIndex(it);
        for (IndexType i = 0; i < index; ++i) {
            sum += child_sums[i];
        }
        this->pointers[index]->sum_inclusive(it, sum);
    }

    void sum_exclusive(uint64_t it, SumType& sum) const {
        IndexType index = this->getIndex(it);
        for (IndexType i = 0; i < index; ++i) {
            sum += child_sums[i];
        }
        this->pointers[index]->sum_exclusive(it, sum);
    }
};

template <typename Parent, typename SumType>
struct SummedNodeInfo : public Parent {
    SumType sum{};

    SummedNodeInfo() noexcept = default;

    template <typename P>
    SummedNodeInfo(P&& p, const bool changed) noexcept : Parent(std::forward<P>(p), changed), sum(changed ? this->ptr->sum() : p->sum()) {}
};
} //end namespace bpptree::detail
