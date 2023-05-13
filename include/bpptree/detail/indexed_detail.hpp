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

    auto find_index(SizeType search_val) const {
        return std::tuple<IndexType, SizeType>(search_val - 1, 0);
    }

    auto insertion_index(SizeType search_val) const {
        return std::tuple<IndexType, SizeType>(search_val, 0);
    }

    Value const& at_index(SizeType search_val) const {
        return this->values[search_val - 1];
    }

    template <typename I>
    void seek_index(
            I& it,
            SizeType index
    ) const {
        this->set_index(it.iter, index - 1);
        it.leaf = &this->self();
    }

    template <typename... Args>
    void compute_delta_insert2(IndexType index, InfoType& node_info, Args const&... args) const {
        node_info.children = 1;
        Parent::compute_delta_insert2(index, node_info, args...);
    }

    template <typename... Args>
    void compute_delta_set2(IndexType index, InfoType& node_info, Args const&... args) const {
        Parent::compute_delta_set2(index, node_info, args...);
    }

    void compute_delta_erase2(IndexType index, InfoType& node_info) const {
        if constexpr (std::is_unsigned_v<SizeType>) {
            node_info.children = std::numeric_limits<SizeType>::max();
        } else {
            node_info.children = -1;
        }
        Parent::compute_delta_erase2(index, node_info);
    }

    SizeType children() const {
        return this->length;
    }

    void order(uint64_t it, SizeType& size) const {
        size += static_cast<SizeType>(this->get_index(it));
    }
};

template <typename Parent, typename Value, typename SizeType, auto internal_size>
struct IndexedInternalNode : public Parent {

    using NodeType = typename Parent::NodeType;

    using ChildType = typename Parent::ChildType;

    template <typename PtrType>
    using InfoType = typename Parent::template InfoType<PtrType>;

    template <typename PtrType>
    using SplitType = typename Parent::template SplitType<PtrType>;

    Array<SizeType, internal_size> child_counts{};

    void move_element2(IndexType dest_index, NodeType& source, IndexType source_index) {
        child_counts[dest_index] = std::move(source.child_counts[source_index]);
        Parent::move_element2(dest_index, source, source_index);
    }

    void copy_element2(IndexType dest_index, NodeType const& source, IndexType source_index) {
        child_counts[dest_index] = source.child_counts[source_index];
        Parent::copy_element2(dest_index, source, source_index);
    }

    void replace_element2(IndexType index, InfoType<ChildType>& t) {
        child_counts[index] += t.children;
        Parent::replace_element2(index, t);
    }

    void set_element2(IndexType index, InfoType<ChildType>& t) {
        child_counts[index] = t.children;
        Parent::set_element2(index, t);
    }

    void compute_delta_split2(SplitType<ChildType> const& split, InfoType<NodeType>& node_info, IndexType index) const {
        node_info.children = split.left.children + split.right.children - child_counts[index];
        Parent::compute_delta_split2(split, node_info, index);
    }

    void compute_delta_replace2(InfoType<ChildType> const& update, InfoType<NodeType>& node_info, IndexType index) const {
        node_info.children = update.children;
        Parent::compute_delta_replace2(update, node_info, index);
    }

    void compute_delta_erase2(IndexType index, InfoType<NodeType>& node_info) const {
        if constexpr (std::is_unsigned_v<SizeType>) {
            node_info.children = ~child_counts[index] + 1;
        } else {
            node_info.children = -child_counts[index];
        }
        Parent::compute_delta_erase2(index, node_info);
    }

    auto find_index(SizeType search_val) const {
        std::tuple<IndexType, SizeType> ret(0, search_val);
        auto& [index, remainder] = ret;
        while (index < this->length - 1) {
            if (child_counts[index] < remainder) {
                remainder -= child_counts[index];
                ++index;
            } else {
                break;
            }
        }
        return ret;
    }

    auto insertion_index(SizeType search_val) const {
        return find_index(search_val);
    }

    Value const& at_index(SizeType search_val) const {
        auto [index, remainder] = find_index(search_val);
        return this->pointers[index]->at_index(remainder);
    }

    template <typename I>
    void seek_index(
            I& it,
            SizeType search_val
    ) const {
        auto [index, remainder] = find_index(search_val);
        this->set_index(it.iter, index);
        this->pointers[this->get_index(it.iter)]->seek_index(it, remainder);
    }

    SizeType children() const {
        SizeType ret = 0;
        for (IndexType i = 0; i < this->length; ++i) {
            ret += child_counts[i];
        }
        return ret;
    }

    void order(uint64_t it, SizeType& size) const {
        IndexType index = this->get_index(it);
        for (IndexType i = 0; i < index; ++i) {
            size += child_counts[i];
        }
        this->pointers[index]->order(it, size);
    }

    template <typename L>
    ssize advance(L const*& leaf, uint64_t& it, ssize n) const {
        start:
        n = this->pointers[this->get_index(it)]->advance(leaf, it, n);
        if (n > 0) {
            while (this->get_index(it) < this->length - 1) {
                this->inc_index(it);
                if (static_cast<ssize>(child_counts[this->get_index(it)]) < n) {
                    n -= static_cast<ssize>(child_counts[this->get_index(it)]);
                } else {
                    this->pointers[this->get_index(it)]->seek_first(it);
                    --n;
                    goto start;
                }
            }
            return n;
        }
        if (n < 0) {
            while (this->get_index(it) > 0) {
                this->dec_index(it);
                if (static_cast<ssize>(child_counts[this->get_index(it)]) < -n) {
                    n += static_cast<ssize>(child_counts[this->get_index(it)]);
                } else {
                    this->pointers[this->get_index(it)]->seek_last(it);
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

    IndexedNodeInfo() = default;

    template <typename P>
    IndexedNodeInfo(P const& p, const bool changed) : Parent(p, changed), children(p->children()) {}
};
}
