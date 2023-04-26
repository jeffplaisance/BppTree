//
// B++ Tree: A B+ Tree library written in C++
// Copyright (C) 2023 Jeff Plaisance
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#pragma once

#include <tuple>
#include <cstdint>
#include "helpers.hpp"

namespace bpptree::detail {

template <typename Parent, typename SumType, typename Extractor>
struct SummedLeafNode : public Parent {

    static constexpr Extractor extractor{};

    using InfoType = typename Parent::InfoType;

    auto find_index_forward(SumType target) const {
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
    void seek_index_forward(
            I& it,
            SumType target
    ) const {
        auto [index, remainder] = find_index_forward(target);
        this->set_index(it.iter, index);
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
        IndexType index = this->get_index(it);
        for (IndexType i = 0; i <= index; ++i) {
            sum += extractor(this->values[i]);
        }
    }

    void sum_exclusive(uint64_t it, SumType& sum) const {
        IndexType index = this->get_index(it);
        for (IndexType i = 0; i < index; ++i) {
            sum += extractor(this->values[i]);
        }
    }

    template <typename... Args>
    void compute_delta_insert2(IndexType index, InfoType& node_info, Args const&... args) const {
        node_info.sum = extractor(args...);
        Parent::compute_delta_insert2(index, node_info, args...);
    }

    template <typename... Args>
    void compute_delta_set2(IndexType index, InfoType& node_info, Args const&... args) const {
        node_info.sum = extractor(args...) - extractor(this->values[index]);
        Parent::compute_delta_set2(index, node_info, args...);
    }

    void compute_delta_erase2(IndexType index, InfoType& node_info) const {
        if constexpr (std::is_unsigned_v<SumType>) {
            node_info.sum = ~extractor(this->values[index]) + 1;
        } else {
            node_info.sum = -extractor(this->values[index]);
        }
        Parent::compute_delta_erase2(index, node_info);
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

    Array<SumType, internal_size> child_sums{};

    void move_element2(IndexType dest_index, NodeType& source, IndexType source_index) {
        child_sums[dest_index] = std::move(source.child_sums[source_index]);
        Parent::move_element2(dest_index, source, source_index);
    }

    void copy_element2(IndexType dest_index, NodeType const& source, IndexType source_index) {
        child_sums[dest_index] = source.child_sums[source_index];
        Parent::copy_element2(dest_index, source, source_index);
    }

    void replace_element2(IndexType index, InfoType<ChildType>& t) {
        child_sums[index] += t.sum;
        Parent::replace_element2(index, t);
    }

    void set_element2(IndexType index, InfoType<ChildType>& t) {
        child_sums[index] = std::move(t.sum);
        Parent::set_element2(index, t);
    }

    void compute_delta_split2(SplitType<ChildType> const& split, InfoType<NodeType>& node_info, IndexType index) const {
        node_info.sum = split.left.sum + split.right.sum - child_sums[index];
        Parent::compute_delta_split2(split, node_info, index);
    }

    void compute_delta_replace2(InfoType<ChildType> const& update, InfoType<NodeType>& node_info, IndexType index) const {
        node_info.sum = update.sum;
        Parent::compute_delta_replace2(update, node_info, index);
    }

    void compute_delta_erase2(IndexType index, InfoType<NodeType>& node_info) const {
        if constexpr (std::is_unsigned_v<SumType>) {
            node_info.sum = ~child_sums[index] + 1;
        } else {
            node_info.sum = -child_sums[index];
        }
        Parent::compute_delta_erase2(index, node_info);
    }

    auto find_index_forward(SumType target) const {
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
    void seek_index_forward(
            I& it,
            SumType target
    ) const {
        auto [index, remainder] = find_index_forward(target);
        this->set_index(it.iter, index);
        this->pointers[this->get_index(it.iter)]->seek_index_forward(it, remainder);
    }

    [[nodiscard]] SumType sum() const {
        SumType ret{};
        for (IndexType i = 0; i < this->length; ++i) {
            ret += child_sums[i];
        }
        return ret;
    }

    void sum_inclusive(uint64_t it, SumType& sum) const {
        IndexType index = this->get_index(it);
        for (IndexType i = 0; i < index; ++i) {
            sum += child_sums[i];
        }
        this->pointers[index]->sum_inclusive(it, sum);
    }

    void sum_exclusive(uint64_t it, SumType& sum) const {
        IndexType index = this->get_index(it);
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
