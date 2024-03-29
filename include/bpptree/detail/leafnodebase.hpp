//
// B++ Tree: A B+ Tree library written in C++
// Copyright (C) 2023 Jeff Plaisance
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#pragma once

#include "uninitialized_array.hpp"
#include "common.hpp"

namespace bpptree::detail {

template <typename Parent, typename Value, auto leaf_size, bool disable_exceptions>
struct LeafNodeBase : public Parent {

    using NodeType = typename Parent::SelfType;

    using InfoType = typename Parent::template NodeInfoType<NodeType>;

    using ReplaceType = Replace<InfoType>;

    using SplitType = Split<InfoType>;

    static constexpr int depth = 1;

    static constexpr int it_shift = 0;

    static constexpr int it_bits = bits_required<leaf_size>();

    static constexpr uint64_t it_mask = (1ULL << it_bits) - 1;

    static constexpr uint64_t it_clear = ~(it_mask << it_shift);

    UninitializedArray<Value, leaf_size> values;

    LeafNodeBase() = default;

    LeafNodeBase(LeafNodeBase const& other) : Parent(other), values(other.values, other.length) {}

    LeafNodeBase& operator=(LeafNodeBase const& other) = delete;

    ~LeafNodeBase() {
        for (IndexType i = 0; i < this->length; ++i) {
            values.destruct(i);
        }
    }

    static IndexType get_index(uint64_t it) {
        return (it >> it_shift) & it_mask;
    }

    static void clear_index(uint64_t& it) {
        it = it & it_clear;
    }

    template <typename T, std::enable_if_t<std::is_integral_v<T>, bool> = true>
    static void set_index(uint64_t& it, T const index) noexcept {
        clear_index(it);
        it = it | (static_cast<uint64_t const>(index) << it_shift);
    }

    template <typename... Args>
    void compute_delta_insert(IndexType index, InfoType& node_info, Args const&... args) const {
        this->self().compute_delta_insert2(index, node_info, args...);
    }

    template <typename... Args>
    void compute_delta_insert2(IndexType, InfoType&, Args const&...) const {}

    template <typename... Args>
    void compute_delta_set(IndexType index, InfoType& node_info, Args const&... args) const {
        this->self().compute_delta_set2(index, node_info, args...);
    }

    template <typename... Args>
    void compute_delta_set2(IndexType, InfoType&, Args const&...) const {}

    void compute_delta_erase(IndexType index, InfoType& node_info) const {
        this->self().compute_delta_erase2(index, node_info);
    }

    void compute_delta_erase2(IndexType, InfoType&) const {}

    template <typename... Args>
    void insert_no_split(LeafNodeBase& node, IndexType index, Args&&... args) noexcept(disable_exceptions) {
        for (IndexType i = this->length; i > index; --i) {
            if (this->persistent) {
                node.values.set(i, node.length, values[i - 1]);
            } else {
                node.values.set(i, node.length, values.move(i - 1));
            }
        }
        node.values.emplace(index, node.length, std::forward<Args>(args)...);
        if (this->persistent) {
            for (IndexType i = 0; i < index; ++i) {
                node.values.set(i, node.length, values[i]);
            }
        }
        node.length = this->length + 1;
    }

    template <typename T>
    void set_element(LeafNodeBase& left, LeafNodeBase& right, IndexType index, IndexType split_point, T&& t) {
        if (index < split_point) {
            left.values.set(index, left.length, std::forward<T>(t));
        } else {
            right.values.set(index - split_point, right.length, std::forward<T>(t));
        }
    }

    template <typename... Args>
    bool insert_split(LeafNodeBase& left, LeafNodeBase& right, IndexType index, uint64_t& iter, bool right_most, Args&&... args) noexcept(disable_exceptions) {
        IndexType split_point = right_most && index == leaf_size ? index : (leaf_size + 1) / 2;
        for (IndexType i = this->length; i > index; --i) {
            if (this->persistent) {
                set_element(left, right, i, split_point, values[i - 1]);
            } else {
                set_element(left, right, i, split_point, std::move(values[i - 1]));
            }
        }
        if (index < split_point) {
            left.values.emplace(index, left.length, std::forward<Args>(args)...);
        } else {
            right.values.emplace(index - split_point, right.length, std::forward<Args>(args)...);
        }
        for (IndexType i = this->persistent ? 0 : split_point; i < index; ++i) {
            if (this->persistent) {
                set_element(left, right, i, split_point, values[i]);
            } else {
                set_element(left, right, i, split_point, std::move(values[i]));
            }
        }
        for (IndexType i = split_point; i < left.length; ++i) {
            left.values.destruct(i);
        }
        left.length = static_cast<uint16_t>(split_point);
        right.length = static_cast<uint16_t>(leaf_size + 1 - split_point);
        if (index >= split_point) {
            set_index(iter, index - split_point);
            return false;
        }
        set_index(iter, index);
        return true;
    }

    template <typename R, typename S, typename... Args>
    void insert_index(IndexType index, R&& do_replace, S&& do_split, size_t& size, uint64_t& iter, bool right_most, Args&&... args) noexcept(disable_exceptions) {
        ++size;
        if (this->length != leaf_size) {
            set_index(iter, index);
            ReplaceType replace{};
            compute_delta_insert(index, replace.delta, args...);
            if (this->persistent) {
                replace.delta.ptr = make_ptr<NodeType>();
                replace.delta.ptr_changed = true;
                insert_no_split(*replace.delta.ptr, index, std::forward<Args>(args)...);
                do_replace(replace);
            } else {
                insert_no_split(this->self(), index, std::forward<Args>(args)...);
                do_replace(replace);
            }
        } else {
            auto right = make_ptr<NodeType>();
            if (this->persistent) {
                NodePtr<NodeType> left = make_ptr<NodeType>();
                bool new_element_left = insert_split(*left, *right, index, iter, right_most, std::forward<Args>(args)...);
                do_split(SplitType(std::move(left), std::move(right), true, new_element_left));
            } else {
                bool new_element_left = insert_split(this->self(), *right, index, iter, right_most, std::forward<Args>(args)...);
                do_split(SplitType(&(this->self()), std::move(right), false, new_element_left));
            }
        }
    }

    template <typename T, typename F, typename R, typename S, typename... Args>
    void insert(T const& search_val, F&& finder, R&& do_replace, S&& do_split, size_t& size, uint64_t& iter, bool right_most, Args&&... args) {
        auto [index, remainder] = finder(this->self(), search_val);
        insert_index(index, do_replace, do_split, size, iter, right_most, std::forward<Args>(args)...);
    }

    template <typename R, typename... Args>
    void assign2(IndexType index, R&& do_replace, uint64_t& iter, Args&&... args) {
        set_index(iter, index);
        ReplaceType replace{};
        compute_delta_set(index, replace.delta, args...);
        if (this->persistent) {
            replace.delta.ptr = make_ptr<NodeType>(this->self());
            replace.delta.ptr_changed = true;
            replace.delta.ptr->values.emplace(index, replace.delta.ptr->length, std::forward<Args>(args)...);
            do_replace(replace);
        } else {
            values.emplace_unchecked(index, std::forward<Args>(args)...);
            do_replace(replace);
        }
    }

    template <typename T, typename F, typename R, typename... Args>
    void assign(T const& search_val, F&& finder, R&& do_replace, uint64_t& iter, Args&&... args) {
        auto [index, remainder] = finder(this->self(), search_val);
        assign2(index, do_replace, iter, std::forward<Args>(args)...);
    }

    bool erase(LeafNodeBase& node, IndexType index, uint64_t& iter, bool right_most) {
        if (this->persistent) {
            for (IndexType i = 0; i < index; ++i) {
                node.values.set(i, node.length, values[i]);
            }
        }
        for (IndexType i = index + 1; i < this->length; ++i) {
            if (this->persistent) {
                node.values.set(i - 1, node.length, values[i]);
            } else {
                node.values.set(i - 1, node.length, values.move(i));
            }
        }
        if (node.length == this->length) {
            node.values.destruct(this->length - 1);
        }
        node.length = this->length - 1;
        bool carry = index == node.length && !right_most;
        set_index(iter, carry ? 0 : index);
        return carry;
    }

    template <typename R, typename E>
    void erase(IndexType index, R&& do_replace, E&& do_erase, uint64_t& iter, bool right_most) {
        if (this->length > 1) {
            ReplaceType replace{};
            compute_delta_erase(index, replace.delta);
            if (this->persistent) {
                replace.delta.ptr = make_ptr<NodeType>();
                replace.delta.ptr_changed = true;
                replace.carry = erase(*replace.delta.ptr, index, iter, right_most);
                do_replace(replace);
            } else {
                replace.carry = erase(this->self(), index, iter, right_most);
                do_replace(replace);
            }
        } else {
            set_index(iter, 0);
            do_erase();
        }
    }

    template <typename T, typename F, typename R, typename E>
    void erase(T const& search_val, F&& finder, R&& do_replace, E&& do_erase, size_t& size, uint64_t& iter, bool right_most) {
        auto [index, remainder] = finder(this->self(), search_val);
        --size;
        erase(index, do_replace, do_erase, iter, right_most);
    }

    template <typename T, typename F, typename R, typename U>
    void update(T const& search_val, F&& finder, R&& do_replace, uint64_t& iter, U&& updater) {
        auto [index, remainder] = finder(this->self(), search_val);
        assign2(index, do_replace, iter, updater(std::as_const(values[index])));
    }

    template <typename T, typename F, typename R, typename U>
    void update2(T const& search_val, F&& finder, R&& do_replace, uint64_t& iter, U&& updater) {
        auto [index, remainder] = finder(this->self(), search_val);
        updater(
                [this, index = index, &do_replace, &iter](auto&&... args)
                // clang incorrectly warns on unused capture without this-> before assign2
                { this->assign2(index, do_replace, iter, std::forward<decltype(args)>(args)...); },
                std::as_const(values[index]));
    }

    Value const& get_iter(uint64_t it) const {
        return values[get_index(it)];
    }

    void make_persistent() {
        this->persistent = true;
    }

    void seek_first(uint64_t& it) const {
        clear_index(it);
    }

    void seek_last(uint64_t& it) const {
        set_index(it, this->length - 1);
    }

    void seek_end(uint64_t& it) const {
        set_index(it, this->length);
    }

    void seek_begin(typename LeafNodeBase::SelfType const*& leaf, uint64_t& it) const {
        clear_index(it);
        leaf = &this->self();
    }

    void seek_end(typename LeafNodeBase::SelfType const*& leaf, uint64_t& it) const {
        set_index(it, this->length);
        leaf = &this->self();
    }

    Value const& front() const {
        return values[0];
    }

    Value const& back() const {
        return values[this->length - 1];
    }

    ssize advance(typename LeafNodeBase::SelfType const*& leaf, uint64_t& it, ssize n) const {
        //if n == 0 then we are just fixing the leaf pointer
        if (n == 0) {
            leaf = &this->self();
            return 0;
        }
        auto sum = get_index(it) + n;
        if (sum >= this->length) {
            auto ret = sum - (this->length - 1);
            set_index(it, this->length - 1);
            return ret;
        }
        if (sum < 0) {
            clear_index(it);
            return sum;
        }
        leaf = &this->self();
        set_index(it, sum);
        return 0;
    }
};
} //end namespace bpptree::detail
