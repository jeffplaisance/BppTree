//
// B++ Tree: A B+ Tree library written in C++
// Copyright (C) 2023 Jeff Plaisance
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#pragma once

#include "uninitialized_array.hpp"

namespace bpptree::detail {

template <typename Parent, typename Value, auto LeafSize>
struct LeafNodeBase : public Parent {

    using NodeType = typename Parent::SelfType;

    using InfoType = typename Parent::template NodeInfoType<NodeType>;

    using ReplaceType = Replace<InfoType>;

    using SplitType = Split<InfoType>;

    static constexpr int depth = 1;

    static constexpr int itShift = 0;

    static constexpr int itBits = bits_required<LeafSize>();

    static constexpr uint64_t itMask = (1ULL << itBits) - 1;

    static constexpr uint64_t itClear = ~(itMask << itShift);

    uint16_t length = 0;
    bool persistent = false;
    UninitializedArray<Value, LeafSize> values;

    LeafNodeBase() noexcept = default;

    LeafNodeBase(LeafNodeBase const& other) noexcept : length(other.length), persistent(other.persistent), values(other.values, other.length) {}

    ~LeafNodeBase() {
        if constexpr (!std::is_trivially_destructible_v<Value>) {
            for (IndexType i = 0; i < length; ++i) {
                values.destruct(i);
            }
        }
    }

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

    template <typename... Args>
    void computeDeltaInsert(IndexType index, InfoType& nodeInfo, Args const&... args) const {
        this->self().computeDeltaInsert2(index, nodeInfo, args...);
    }

    template <typename... Args>
    void computeDeltaInsert2(IndexType, InfoType&, Args const&...) const {}

    template <typename... Args>
    void computeDeltaSet(IndexType index, InfoType& nodeInfo, Args const&... args) const {
        this->self().computeDeltaSet2(index, nodeInfo, args...);
    }

    template <typename... Args>
    void computeDeltaSet2(IndexType, InfoType&, Args const&...) const {}

    void computeDeltaErase(IndexType index, InfoType& nodeInfo) const {
        this->self().computeDeltaErase2(index, nodeInfo);
    }

    void computeDeltaErase2(IndexType, InfoType&) const {}

    template <typename... Args>
    void insertNoSplit(LeafNodeBase& node, IndexType index, Args&&... args) {
        for (IndexType i = length - 1; i >= index; --i) {
            if (persistent) {
                node.values.set(i + 1, node.length, values[i]);
            } else {
                node.values.set(i + 1, node.length, values.move(i));
            }
        }
        node.values.emplace(index, node.length, std::forward<Args>(args)...);
        if (persistent) {
            for (IndexType i = 0; i < index; ++i) {
                node.values.set(i, node.length, values[i]);
            }
        }
        node.length = length + 1;
    }

    template <typename T>
    void setElement(LeafNodeBase& left, LeafNodeBase& right, IndexType index, IndexType split_point, T&& t) {
        if (index < split_point) {
            left.values.set(index, left.length, std::forward<T>(t));
        } else {
            right.values.set(index - split_point, right.length, std::forward<T>(t));
        }
    }

    template <typename... Args>
    bool insertSplit(LeafNodeBase& left, LeafNodeBase& right, IndexType index, uint64_t& iter, bool rightMost, Args&&... args) {
        IndexType split_point = rightMost && index == LeafSize ? index : (LeafSize + 1) / 2;
        for (IndexType i = length - 1; i >= index; --i) {
            if (persistent) {
                setElement(left, right, i + 1, split_point, values[i]);
            } else {
                setElement(left, right, i + 1, split_point, std::move(values[i]));
            }
        }
        if (index < split_point) {
            left.values.emplace(index, left.length, std::forward<Args>(args)...);
        } else {
            right.values.emplace(index - split_point, right.length, std::forward<Args>(args)...);
        }
        for (IndexType i = persistent ? 0 : split_point; i < index; ++i) {
            if (persistent) {
                setElement(left, right, i, split_point, values[i]);
            } else {
                setElement(left, right, i, split_point, std::move(values[i]));
            }
        }
        if constexpr (!std::is_trivially_destructible_v<Value>) {
            for (IndexType i = split_point; i < left.length; ++i) {
                left.values.destruct(i);
            }
        }
        left.length = static_cast<uint16_t>(split_point);
        right.length = static_cast<uint16_t>(LeafSize + 1 - split_point);
        if (index >= split_point) {
            setIndex(iter, index - split_point);
            return false;
        }
        setIndex(iter, index);
        return true;
    }

    template <typename R, typename S, typename... Args>
    void insertIndex(IndexType index, R&& doReplace, S&& doSplit, size_t& size, uint64_t& iter, bool rightMost, Args&&... args) {
        ++size;
        if (length != LeafSize) {
            setIndex(iter, index);
            ReplaceType replace{};
            computeDeltaInsert(index, replace.delta, args...);
            if (persistent) {
                replace.delta.ptr = makePtr<NodeType>();
                replace.delta.ptrChanged = true;
                insertNoSplit(*replace.delta.ptr, index, std::forward<Args>(args)...);
                doReplace(replace);
            } else {
                insertNoSplit(this->self(), index, std::forward<Args>(args)...);
                doReplace(replace);
            }
        } else {
            auto right = makePtr<NodeType>();
            if (persistent) {
                NodePtr<NodeType> left = makePtr<NodeType>();
                bool new_element_left = insertSplit(*left, *right, index, iter, rightMost, std::forward<Args>(args)...);
                doSplit(SplitType(std::move(left), std::move(right), true, new_element_left));
            } else {
                bool new_element_left = insertSplit(this->self(), *right, index, iter, rightMost, std::forward<Args>(args)...);
                doSplit(SplitType(&(this->self()), std::move(right), false, new_element_left));
            }
        }
    }

    template <typename T, typename F, typename R, typename S, typename... Args>
    void insert(T const& searchVal, F&& finder, R&& doReplace, S&& doSplit, size_t& size, uint64_t& iter, bool rightMost, Args&&... args) {
        auto [index, remainder] = finder(this->self(), searchVal);
        insertIndex(index, doReplace, doSplit, size, iter, rightMost, std::forward<Args>(args)...);
    }

    template <typename R, typename... Args>
    void assign2(IndexType index, R&& doReplace, uint64_t& iter, Args&&... args) {
        setIndex(iter, index);
        ReplaceType replace{};
        computeDeltaSet(index, replace.delta, args...);
        if (persistent) {
            replace.delta.ptr = makePtr<NodeType>(this->self(), false);
            replace.delta.ptrChanged = true;
            replace.delta.ptr->values.emplace(index, replace.delta.ptr->length, std::forward<Args>(args)...);
            doReplace(replace);
        } else {
            values.emplace_unchecked(index, std::forward<Args>(args)...);
            doReplace(replace);
        }
    }

    template <typename T, typename F, typename R, typename... Args>
    void assign(T const& searchVal, F&& finder, R&& doReplace, uint64_t& iter, Args&&... args) {
        auto [index, remainder] = finder(this->self(), searchVal);
        assign2(index, doReplace, iter, std::forward<Args>(args)...);
    }

    bool erase(LeafNodeBase& node, IndexType index, uint64_t& iter, bool rightMost) {
        if (persistent) {
            for (IndexType i = 0; i < index; ++i) {
                node.values.set(i, node.length, values[i]);
            }
        }
        for (IndexType i = index + 1; i < length; ++i) {
            if (persistent) {
                node.values.set(i - 1, node.length, values[i]);
            } else {
                node.values.set(i - 1, node.length, values.move(i));
            }
        }
        if (node.length == length) {
            node.values.destruct(length - 1);
        }
        node.length = length - 1;
        bool carry = index == node.length && !rightMost;
        setIndex(iter, carry ? 0 : index);
        return carry;
    }

    template <typename R, typename E>
    void erase(IndexType index, R&& doReplace, E&& doErase, uint64_t& iter, bool rightMost) {
        if (length > 1) {
            ReplaceType replace{};
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
        --size;
        erase(index, doReplace, doErase, iter, rightMost);
    }

    template <typename T, typename F, typename R, typename U>
    void update(T const& searchVal, F&& finder, R&& doReplace, uint64_t& iter, U&& updater) {
        auto [index, remainder] = finder(this->self(), searchVal);
        assign2(index, doReplace, iter, updater(std::as_const(values[index])));
    }

    template <typename T, typename F, typename R, typename U>
    void update2(T const& searchVal, F&& finder, R&& doReplace, uint64_t& iter, U&& updater) {
        auto [index, remainder] = finder(this->self(), searchVal);
        updater(
                [this, index = index, &doReplace, &iter](auto&&... args)
                // clang incorrectly warns on unused capture without this-> before assign2
                { this->assign2(index, doReplace, iter, std::forward<decltype(args)>(args)...); },
                std::as_const(values[index]));
    }

    Value const& getIter(uint64_t it) const {
        return values[getIndex(it)];
    }

    void makePersistent() {
        persistent = true;
    }

    void seekFirst(uint64_t& it) const {
        clearIndex(it);
    }

    void seekLast(uint64_t& it) const {
        setIndex(it, length - 1);
    }

    void seekEnd(uint64_t& it) const {
        setIndex(it, length);
    }

    void seekBegin(typename LeafNodeBase::SelfType const*& leaf, uint64_t& it) const {
        clearIndex(it);
        leaf = &this->self();
    }

    void seekEnd(typename LeafNodeBase::SelfType const*& leaf, uint64_t& it) const {
        setIndex(it, length);
        leaf = &this->self();
    }

    Value const& front() const {
        return values[0];
    }

    Value const& back() const {
        return values[length - 1];
    }

    ssize advance(typename LeafNodeBase::SelfType const*& leaf, uint64_t& it, ssize n) const {
        auto sum = getIndex(it) + n;
        if (sum >= length) {
            auto ret = sum - (length - 1);
            setIndex(it, length - 1);
            return ret;
        }
        if (sum < 0) {
            clearIndex(it);
            return sum;
        }
        leaf = &this->self();
        setIndex(it, sum);
        return 0;
    }

    void getIndexes(uint64_t it, std::vector<uint16_t>& indexes) const {
        IndexType index = getIndex(it);
        indexes.emplace_back(index);
    }
};
} //end namespace bpptree::detail
