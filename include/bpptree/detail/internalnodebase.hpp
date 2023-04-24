//
// B++ Tree: A B+ Tree library written in C++
// Copyright (C) 2023 Jeff Plaisance
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#pragma once

#include "nodeptr.hpp"

namespace bpptree::detail {

template <typename Parent, typename Value, auto LeafSize, auto InternalSize, auto Depth>
struct InternalNodeBase : public Parent {

    using NodeType = typename Parent::SelfType;

    using ChildType = std::conditional_t<(Depth > 2),
            typename Parent::template InternalNodeType<LeafSize, InternalSize, Depth - 1>,
            typename Parent::template LeafNodeType<LeafSize>>;

    template <typename PtrType>
    using InfoType = typename Parent::template NodeInfoType<PtrType>;

    template <typename PtrType>
    using ReplaceType = Replace<InfoType<PtrType>>;

    template <typename PtrType>
    using SplitType = Split<InfoType<PtrType>>;

    static constexpr int depth = Depth;

    static constexpr int itShift = ChildType::itShift + ChildType::itBits;

    static constexpr int itBits = bits_required<InternalSize>();

    static constexpr uint64_t itMask = (1ULL << itBits) - 1;

    static constexpr uint64_t itClear = ~(itMask << itShift);

    uint16_t length = 0;
    bool persistent = false;
    safe_array<NodePtr<ChildType>, InternalSize> pointers{};

    static IndexType getIndex(uint64_t it) noexcept {
        return (it >> itShift) & itMask;
    }

    static void clearIndex(uint64_t& it) noexcept {
        it = it & itClear;
    }

    static void setIndex(uint64_t& it, uint64_t index) noexcept {
        clearIndex(it);
        it = it | (index << itShift);
    }

    template <typename T, std::enable_if_t<std::is_integral_v<T>, bool> = true>
    static void setIndex(uint64_t& it, T const t) noexcept {
        setIndex(it, static_cast<uint64_t const>(t));
    }

    static void incIndex(uint64_t& it) noexcept {
        it += 1ULL << itShift;
    }

    static void decIndex(uint64_t& it) noexcept {
        it -= 1ULL << itShift;
    }

    void eraseElement(IndexType index) {
        this->self().eraseElement2(index);
    }

    void eraseElement2(IndexType index) {
        pointers[index] = nullptr;
    }

    void moveElement(IndexType destIndex, NodeType& source, IndexType sourceIndex) {
        this->self().moveElement2(destIndex, source, sourceIndex);
    }

    void moveElement2(IndexType destIndex, NodeType& source, IndexType sourceIndex) {
        pointers[destIndex] = std::move(source.pointers[sourceIndex]);
    }

    void copyElement(IndexType destIndex, NodeType const& source, IndexType sourceIndex) {
        this->self().copyElement2(destIndex, source, sourceIndex);
    }

    void copyElement2(IndexType destIndex, NodeType const& source, IndexType sourceIndex) {
        pointers[destIndex] = source.pointers[sourceIndex];
    }

    void replaceElement(IndexType index, InfoType<ChildType>& t) {
        this->self().replaceElement2(index, t);
    }

    void replaceElement2(IndexType index, InfoType<ChildType>& t) {
        if (t.ptrChanged) {
            pointers[index] = std::move(t.ptr);
        }
    }

    void setElement(IndexType index, InfoType<ChildType>& t) {
        this->self().setElement2(index, t);
    }

    void setElement2(IndexType index, InfoType<ChildType>& t) {
        if (t.ptrChanged) {
            pointers[index] = std::move(t.ptr);
        }
    }

    void computeDeltaSplit(SplitType<ChildType> const& split, InfoType<NodeType>& nodeInfo, IndexType index) const {
        this->self().computeDeltaSplit2(split, nodeInfo, index);
    }

    void computeDeltaSplit2(SplitType<ChildType> const&, InfoType<NodeType>&, IndexType) const {}

    void computeDeltaReplace(InfoType<ChildType> const& update, InfoType<NodeType>& nodeInfo, IndexType index) const {
        this->self().computeDeltaReplace2(update, nodeInfo, index);
    }

    void computeDeltaReplace2(InfoType<ChildType> const&, InfoType<NodeType>&, IndexType) const {}

    void computeDeltaErase(IndexType index, InfoType<NodeType>& nodeInfo) const {
        this->self().computeDeltaErase2(index, nodeInfo);
    }

    void computeDeltaErase2(IndexType, InfoType<NodeType>&) const {}

    template <typename R>
    void insertReplace(IndexType index, ReplaceType<ChildType>& replace, R&& doReplace, uint64_t& iter) {
        ReplaceType<NodeType> result{};
        result.carry = replace.carry && index == length - 1;
        setIndex(iter, !replace.carry ? index : result.carry ? 0 : index + 1);
        computeDeltaReplace(replace.delta, result.delta, index);
        if (persistent) {
            result.delta.ptr = makePtr<NodeType>(this->self(), false);
            result.delta.ptr->replaceElement(index, replace.delta);
            result.delta.ptrChanged = true;
            doReplace(result);
        } else {
            replaceElement(index, replace.delta);
            doReplace(result);
        }
    }

    void insertSplitReplace(InternalNodeBase& node, IndexType index, SplitType<ChildType>& split) {
        for (IndexType i = length - 1; i > index; --i) {
            if (persistent) {
                node.copyElement(i + 1, this->self(), i);
            } else {
                node.moveElement(i + 1, this->self(), i);
            }
        }
        node.setElement(index, split.left);
        node.setElement(index + 1, split.right);
        if (persistent) {
            for (IndexType i = 0; i < index; ++i) {
                node.copyElement(i, this->self(), i);
            }
        }
        node.length = length + 1;
    }

    template <typename F>
    static void setElement2(InternalNodeBase& left, InternalNodeBase& right, IndexType index, IndexType split_point, F&& f) {
        if (index < split_point) {
            f(left, index);
        } else {
            f(right, index - split_point);
        }
    }

    void setElement(InternalNodeBase& left, InternalNodeBase& right, IndexType index, IndexType splitPoint, InfoType<ChildType>& update) {
        setElement2(left, right, index, splitPoint,
                    [&update](InternalNodeBase& node, IndexType destIndex)
                    { node.setElement(destIndex, update); }
        );
    }

    void copyElement(InternalNodeBase& left, InternalNodeBase& right, IndexType destIndex, NodeType& source, IndexType sourceIndex, IndexType splitPoint) {
        setElement2(left, right, destIndex, splitPoint,
                    [&source, sourceIndex](InternalNodeBase& node, IndexType destIndex)
                    { node.copyElement(destIndex, source, sourceIndex); }
        );
    }

    void moveElement(InternalNodeBase& left, InternalNodeBase& right, IndexType destIndex, NodeType& source, IndexType sourceIndex, IndexType splitPoint) {
        setElement2(left, right, destIndex, splitPoint,
                    [&source, sourceIndex](InternalNodeBase& node, IndexType destIndex)
                    { node.moveElement(destIndex, source, sourceIndex); }
        );
    }

    bool insertSplitSplit(InternalNodeBase& left, InternalNodeBase& right, IndexType index, SplitType<ChildType>& split, uint64_t& iter, bool rightMost) {
        IndexType split_point = rightMost && index + 1 == InternalSize ? index + 1 : (InternalSize + 1) / 2;
        for (IndexType i = length - 1; i > index; --i) {
            if (persistent) {
                copyElement(left, right, i + 1, this->self(), i, split_point);
            } else {
                moveElement(left, right, i + 1, this->self(), i, split_point);
            }
        }
        if (!split.left.ptrChanged) {
            split.left.ptr = std::move(pointers[index]);
            split.left.ptrChanged = true;
        }
        setElement(left, right, index, split_point, split.left);
        setElement(left, right, index + 1, split_point, split.right);
        for (IndexType i = persistent ? 0 : split_point; i < index; ++i) {
            if (persistent) {
                copyElement(left, right, i, this->self(), i, split_point);
            } else {
                moveElement(left, right, i, this->self(), i, split_point);
            }
        }
        for (IndexType i = split_point; i < left.length; ++i) {
            left.eraseElement(i);
        }
        left.length = static_cast<uint16_t>(split_point);
        right.length = static_cast<uint16_t>(InternalSize + 1 - split_point);
        IndexType insertion_index = split.new_element_left ? index : index + 1;
        if (insertion_index >= split_point) {
            setIndex(iter, insertion_index - split_point);
            return false;
        }
        setIndex(iter, insertion_index);
        return true;
    }

    template <typename R, typename S>
    void insertSplit(
            IndexType index,
            SplitType<ChildType>& split,
            R&& doReplace,
            S&& doSplit,
            uint64_t& iter,
            bool rightMost
    ) {
        if (length != InternalSize) {
            setIndex(iter, split.new_element_left ? index : index + 1);
            ReplaceType<NodeType> replace{};
            computeDeltaSplit(split, replace.delta, index);
            if (persistent) {
                replace.delta.ptr = makePtr<NodeType>();
                insertSplitReplace(*replace.delta.ptr, index, split);
                replace.delta.ptrChanged = true;
                doReplace(replace);
            } else {
                insertSplitReplace(this->self(), index, split);
                doReplace(replace);
            }
        } else {
            auto right = makePtr<NodeType>();
            if (persistent) {
                auto left = makePtr<NodeType>();
                bool new_element_left = insertSplitSplit(*left, *right, index, split, iter, rightMost);
                doSplit(SplitType<NodeType>(std::move(left), std::move(right), true, new_element_left));
            } else {
                bool new_element_left = insertSplitSplit(this->self(), *right, index, split, iter, rightMost);
                doSplit(SplitType<NodeType>(&(this->self()), std::move(right), false, new_element_left));
            }
        }
    }

    template <typename T, typename F, typename R, typename S, typename... Args>
    void insert(T const& searchVal, F&& finder, R&& doReplace, S&& doSplit, size_t& size, uint64_t& iter, bool rightMost, Args&&... args) {
        auto [index, remainder] = finder(this->self(), searchVal);
        pointers[index]->insert(remainder,
                                finder,
                                [this, index = index, &doReplace, &iter](auto&& replace)
                                { this->insertReplace(index, replace, doReplace, iter); },
                                [this, index = index, &doReplace, &doSplit, &iter, rightMost](auto&& split)
                                { this->insertSplit(index, split, doReplace, doSplit, iter, rightMost); },
                                size,
                                iter,
                                rightMost && index == length - 1,
                                std::forward<Args>(args)...);
    }

    template <typename T, typename F, typename R, typename... Args>
    void assign(T const& searchVal, F&& finder, R&& doReplace, uint64_t& iter, Args&&... args) {
        auto [index, remainder] = finder(this->self(), searchVal);
        pointers[index]->assign(remainder,
                 finder,
                 [this, index = index, &doReplace, &iter](auto&& replace)
                 { this->insertReplace(index, replace, doReplace, iter); },
                 iter,
                 std::forward<Args>(args)...
        );
    }

    bool erase(InternalNodeBase& node, IndexType index, uint64_t& iter, bool rightMost) {
        if (persistent) {
            for (IndexType i = 0; i < index; ++i) {
                node.copyElement(i, this->self(), i);
            }
        }
        for (IndexType i = index + 1; i < length; ++i) {
            if (persistent) {
                node.copyElement(i - 1, this->self(), i);
            } else {
                node.moveElement(i - 1, this->self(), i);
            }
        }
        if (node.length == length) {
            node.eraseElement(length - 1);
        }
        node.length = length - 1;
        bool carry = index == node.length;
        if (carry && rightMost) {
            node.seekEnd(iter);
            return false;
        }
        setIndex(iter, carry ? 0 : index);
        return carry;
    }

    template <typename R, typename E>
    void erase(IndexType index, R&& doReplace, E&& doErase, uint64_t& iter, bool rightMost) {
        if (length > 1) {
            ReplaceType<NodeType> replace{};
            computeDeltaErase(index, replace.delta);
            if (persistent) {
                replace.delta.ptr = makePtr<NodeType>();
                replace.delta.ptrChanged = true;
                replace.carry = erase(*replace.delta.ptr, index, iter, rightMost);
                doReplace(replace);
            } else {
                replace.carry = erase(this->self(), index, iter, rightMost);
                doReplace(replace);
            }
        } else {
            setIndex(iter, 0);
            doErase();
        }
    }

    template <typename T, typename F, typename R, typename E>
    void erase(T const& searchVal, F&& finder, R&& doReplace, E&& doErase, size_t& size, uint64_t& iter, bool rightMost) {
        auto [index, remainder] = finder(this->self(), searchVal);
        pointers[index]->erase(remainder,
                               finder,
                               [this, index = index, &doReplace, &iter](auto&& replace)
                               { this->insertReplace(index, replace, doReplace, iter); },
                               [this, index = index, &doReplace, &doErase, &iter, rightMost]()
                               { this->erase(index, doReplace, doErase, iter, rightMost); },
                               size,
                               iter,
                               rightMost && index == length - 1);
    }

    template <typename T, typename F, typename R, typename U>
    void update(T const& searchVal, F&& finder, R&& doReplace, uint64_t& iter, U&& updater) {
        auto [index, remainder] = finder(this->self(), searchVal);
        pointers[index]->update(remainder,
                                finder,
                                [this, index = index, &doReplace, &iter](auto&& replace)
                                { this->insertReplace(index, replace, doReplace, iter); },
                                iter,
                                updater
        );
    }

    template <typename T, typename F, typename R, typename U>
    void update2(T const& searchVal, F&& finder, R&& doReplace, uint64_t& iter, U&& updater) {
        auto [index, remainder] = finder(this->self(), searchVal);
        pointers[index]->update2(remainder,
                                finder,
                                [this, index = index, &doReplace, &iter](auto&& replace)
                                { this->insertReplace(index, replace, doReplace, iter); },
                                iter,
                                updater
        );
    }

    Value const& getIter(uint64_t it) const {
        return pointers[getIndex(it)]->getIter(it);
    }

    void makePersistent() {
        if (!persistent) {
            persistent = true;
            for (IndexType i = 0; i < length; ++i) {
                pointers[i]->makePersistent();
            }
        }
    }

    void seekFirst(uint64_t& it) const {
        clearIndex(it);
        pointers[0]->seekFirst(it);
    }

    void seekLast(uint64_t& it) const {
        setIndex(it, length - 1);
        pointers[length - 1]->seekLast(it);
    }

    void seekEnd(uint64_t& it) const {
        setIndex(it, length - 1);
        pointers[length - 1]->seekEnd(it);
    }

    void seekBegin(typename InternalNodeBase::template LeafNodeType<LeafSize> const*& leaf, uint64_t& it) const {
        clearIndex(it);
        pointers[0]->seekBegin(leaf, it);
    }

    void seekEnd(typename InternalNodeBase::template LeafNodeType<LeafSize> const*& leaf, uint64_t& it) const {
        setIndex(it, length - 1);
        pointers[length - 1]->seekEnd(leaf, it);
    }

    Value const& front() const {
        return pointers[0]->front();
    }

    Value const& back() const {
        return pointers[length - 1]->back();
    }

    ssize advance(typename InternalNodeBase::template LeafNodeType<LeafSize> const*& leaf, uint64_t& it, ssize n) const {
        while (true) {
            n = pointers[getIndex(it)]->advance(leaf, it, n);
            if (n > 0) {
                if (getIndex(it) < length - 1) {
                    incIndex(it);
                    pointers[getIndex(it)]->seekFirst(it);
                    --n;
                    continue;
                }
                return n;
            }
            if (n < 0) {
                if (getIndex(it) > 0) {
                    decIndex(it);
                    pointers[getIndex(it)]->seekLast(it);
                    ++n;
                    continue;
                }
                return n;
            }
            return 0;
        }
    }

    void getIndexes(uint64_t it, std::vector<uint16_t>& indexes) const {
        IndexType index = getIndex(it);
        indexes.emplace_back(index);
        pointers[index]->getIndexes(it, indexes);
    }
};
} //end namespace bpptree::detail
