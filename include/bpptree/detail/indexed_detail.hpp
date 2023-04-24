//
// B++ Tree: A B+ Tree library written in C++
// Copyright (C) 2023 Jeff Plaisance
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#pragma once

#include <cstdint>
#include <tuple>
#include "helpers.hpp"

namespace bpptree::detail {

template <typename Parent, typename Value, typename SizeType>
struct IndexedLeafNode : public Parent {

    using InfoType = typename Parent::InfoType;

    auto findIndex(SizeType searchVal) const {
        return std::tuple<IndexType, SizeType>(searchVal - 1, 0);
    }

    auto insertionIndex(SizeType searchVal) const {
        return std::tuple<IndexType, SizeType>(searchVal, 0);
    }

    Value const& at_index(SizeType searchVal) const {
        return this->values[searchVal - 1];
    }

    template <typename I>
    void seekIndex(
            I& it,
            SizeType index
    ) const {
        this->setIndex(it.iter, index - 1);
        it.leaf = &this->self();
    }

    template <typename... Args>
    void computeDeltaInsert2(IndexType index, InfoType& nodeInfo, Args const&... args) const {
        nodeInfo.children = 1;
        Parent::computeDeltaInsert2(index, nodeInfo, args...);
    }

    template <typename... Args>
    void computeDeltaSet2(IndexType index, InfoType& nodeInfo, Args const&... args) const {
        Parent::computeDeltaSet2(index, nodeInfo, args...);
    }

    void computeDeltaErase2(IndexType index, InfoType& nodeInfo) const {
        if constexpr (std::is_unsigned_v<SizeType>) {
            nodeInfo.children = std::numeric_limits<SizeType>::max();
        } else {
            nodeInfo.children = -1;
        }
        Parent::computeDeltaErase2(index, nodeInfo);
    }

    SizeType children() {
        return this->length;
    }

    void order(uint64_t it, SizeType& size) const {
        size += static_cast<SizeType>(this->getIndex(it));
    }
};

template <typename Parent, typename Value, typename SizeType, auto InternalSize>
struct IndexedInternalNode : public Parent {

    using NodeType = typename Parent::NodeType;

    using ChildType = typename Parent::ChildType;

    template <typename PtrType>
    using InfoType = typename Parent::template InfoType<PtrType>;

    template <typename PtrType>
    using SplitType = typename Parent::template SplitType<PtrType>;

    safe_array<SizeType, InternalSize> childCounts{};

    void moveElement2(IndexType destIndex, NodeType& source, IndexType sourceIndex) {
        childCounts[destIndex] = std::move(source.childCounts[sourceIndex]);
        Parent::moveElement2(destIndex, source, sourceIndex);
    }

    void copyElement2(IndexType destIndex, NodeType const& source, IndexType sourceIndex) {
        childCounts[destIndex] = source.childCounts[sourceIndex];
        Parent::copyElement2(destIndex, source, sourceIndex);
    }

    void replaceElement2(IndexType index, InfoType<ChildType>& t) {
        childCounts[index] += t.children;
        Parent::replaceElement2(index, t);
    }

    void setElement2(IndexType index, InfoType<ChildType>& t) {
        childCounts[index] = t.children;
        Parent::setElement2(index, t);
    }

    void computeDeltaSplit2(SplitType<ChildType> const& split, InfoType<NodeType>& nodeInfo, IndexType index) const {
        nodeInfo.children = split.left.children + split.right.children - childCounts[index];
        Parent::computeDeltaSplit2(split, nodeInfo, index);
    }

    void computeDeltaReplace2(InfoType<ChildType> const& update, InfoType<NodeType>& nodeInfo, IndexType index) const {
        nodeInfo.children = update.children;
        Parent::computeDeltaReplace2(update, nodeInfo, index);
    }

    void computeDeltaErase2(IndexType index, InfoType<NodeType>& nodeInfo) const {
        if constexpr (std::is_unsigned_v<SizeType>) {
            nodeInfo.children = ~childCounts[index] + 1;
        } else {
            nodeInfo.children = -childCounts[index];
        }
        Parent::computeDeltaErase2(index, nodeInfo);
    }

    auto findIndex(SizeType searchVal) const {
        std::tuple<IndexType, SizeType> ret(0, searchVal);
        auto& [index, remainder] = ret;
        while (index < this->length - 1) {
            if (childCounts[index] < remainder) {
                remainder -= childCounts[index];
                ++index;
            } else {
                break;
            }
        }
        return ret;
    }

    auto insertionIndex(SizeType searchVal) const {
        return findIndex(searchVal);
    }

    Value const& at_index(SizeType searchVal) const {
        auto [index, remainder] = findIndex(searchVal);
        return this->pointers[index]->at_index(remainder);
    }

    template <typename I>
    void seekIndex(
            I& it,
            SizeType searchVal
    ) const {
        auto [index, remainder] = findIndex(searchVal);
        this->setIndex(it.iter, index);
        this->pointers[this->getIndex(it.iter)]->seekIndex(it, remainder);
    }

    SizeType children() {
        SizeType ret = 0;
        for (IndexType i = 0; i < this->length; ++i) {
            ret += childCounts[i];
        }
        return ret;
    }

    void order(uint64_t it, SizeType& size) const {
        IndexType index = this->getIndex(it);
        for (IndexType i = 0; i < index; ++i) {
            size += childCounts[i];
        }
        this->pointers[index]->order(it, size);
    }

    template <typename L>
    ssize advance(L const*& leaf, uint64_t& it, ssize n) const {
        start:
        n = this->pointers[this->getIndex(it)]->advance(leaf, it, n);
        if (n > 0) {
            while (this->getIndex(it) < this->length - 1) {
                this->incIndex(it);
                if (static_cast<size_t>(childCounts[this->getIndex(it)]) < static_cast<size_t>(n)) {
                    n -= childCounts[this->getIndex(it)];
                } else {
                    this->pointers[this->getIndex(it)]->seekFirst(it);
                    --n;
                    goto start;
                }
            }
            return n;
        }
        if (n < 0) {
            while (this->getIndex(it) > 0) {
                this->decIndex(it);
                if (static_cast<size_t>(childCounts[this->getIndex(it)]) < static_cast<size_t>(-n)) {
                    n += childCounts[this->getIndex(it)];
                } else {
                    this->pointers[this->getIndex(it)]->seekLast(it);
                    ++n;
                    goto start;
                }
            }
            return n;
        }
        return 0;
    }
};

template <typename Parent, typename SizeType>
struct IndexedNodeInfo : public Parent {
    SizeType children{};

    IndexedNodeInfo() noexcept = default;

    template <typename P>
    IndexedNodeInfo(P&& p, const bool changed) noexcept : Parent(std::forward<P>(p), changed), children(changed ? this->ptr->children() : p->children()) {}
};
}
