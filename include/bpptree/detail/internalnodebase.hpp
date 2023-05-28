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

template <typename Parent, typename Value, auto leaf_size, auto internal_size, auto depth_v, bool disable_exceptions>
struct InternalNodeBase : public Parent {

    static constexpr int depth = depth_v;

    using NodeType = typename Parent::SelfType;

    using ChildType = std::conditional_t<(depth > 2),
            typename Parent::template InternalNodeType<leaf_size, internal_size, depth - 1>,
            typename Parent::template LeafNodeType<leaf_size>>;

    template <typename PtrType>
    using InfoType = typename Parent::template NodeInfoType<PtrType>;

    template <typename PtrType>
    using ReplaceType = Replace<InfoType<PtrType>>;

    template <typename PtrType>
    using SplitType = Split<InfoType<PtrType>>;

    static constexpr int it_shift = ChildType::it_shift + ChildType::it_bits;

    static constexpr int it_bits = bits_required<internal_size>();

    static constexpr uint64_t it_mask = (1ULL << it_bits) - 1;

    static constexpr uint64_t it_clear = ~(it_mask << it_shift);

    uint16_t length = 0;
    bool persistent = false;
    NodePtr<ChildType> pointers[internal_size]{};

    static IndexType get_index(uint64_t it) {
        return (it >> it_shift) & it_mask;
    }

    static void clear_index(uint64_t& it) {
        it = it & it_clear;
    }

    static void set_index(uint64_t& it, uint64_t index) {
        clear_index(it);
        it = it | (index << it_shift);
    }

    template <typename T, std::enable_if_t<std::is_integral_v<T>, bool> = true>
    static void set_index(uint64_t& it, T const t) {
        set_index(it, static_cast<uint64_t const>(t));
    }

    static void inc_index(uint64_t& it) {
        it += 1ULL << it_shift;
    }

    static void dec_index(uint64_t& it) {
        it -= 1ULL << it_shift;
    }

    void erase_element(IndexType index) {
        this->self().erase_element2(index);
    }

    void erase_element2(IndexType index) {
        pointers[index] = nullptr;
    }

    void move_element(IndexType dest_index, NodeType& source, IndexType source_index) {
        this->self().move_element2(dest_index, source, source_index);
    }

    void move_element2(IndexType dest_index, NodeType& source, IndexType source_index) {
        pointers[dest_index] = std::move(source.pointers[source_index]);
    }

    void copy_element(IndexType dest_index, NodeType const& source, IndexType source_index) {
        this->self().copy_element2(dest_index, source, source_index);
    }

    void copy_element2(IndexType dest_index, NodeType const& source, IndexType source_index) {
        pointers[dest_index] = source.pointers[source_index];
    }

    void replace_element(IndexType index, InfoType<ChildType>& t) {
        this->self().replace_element2(index, t);
    }

    void replace_element2(IndexType index, InfoType<ChildType>& t) {
        if (t.ptr_changed) {
            pointers[index] = std::move(t.ptr);
        }
    }

    void set_element(IndexType index, InfoType<ChildType>& t) {
        this->self().set_element2(index, t);
    }

    void set_element2(IndexType index, InfoType<ChildType>& t) {
        if (t.ptr_changed) {
            pointers[index] = std::move(t.ptr);
        }
    }

    void compute_delta_split(SplitType<ChildType> const& split, InfoType<NodeType>& node_info, IndexType index) const {
        this->self().compute_delta_split2(split, node_info, index);
    }

    void compute_delta_split2(SplitType<ChildType> const&, InfoType<NodeType>&, IndexType) const {}

    void compute_delta_replace(InfoType<ChildType> const& update, InfoType<NodeType>& node_info, IndexType index) const {
        this->self().compute_delta_replace2(update, node_info, index);
    }

    void compute_delta_replace2(InfoType<ChildType> const&, InfoType<NodeType>&, IndexType) const {}

    void compute_delta_erase(IndexType index, InfoType<NodeType>& node_info) const {
        this->self().compute_delta_erase2(index, node_info);
    }

    void compute_delta_erase2(IndexType, InfoType<NodeType>&) const {}

    template <typename R>
    void insert_replace(IndexType index, ReplaceType<ChildType>& replace, R&& do_replace, uint64_t& iter) noexcept(disable_exceptions) {
        ReplaceType<NodeType> result{};
        result.carry = replace.carry && index == length - 1;
        set_index(iter, !replace.carry ? index : result.carry ? 0 : index + 1);
        compute_delta_replace(replace.delta, result.delta, index);
        if (persistent) {
            result.delta.ptr = make_ptr<NodeType>(this->self(), false);
            result.delta.ptr->replace_element(index, replace.delta);
            result.delta.ptr_changed = true;
            do_replace(result);
        } else {
            replace_element(index, replace.delta);
            do_replace(result);
        }
    }

    void insert_split_replace(InternalNodeBase& node, IndexType index, SplitType<ChildType>& split) noexcept(disable_exceptions) {
        for (IndexType i = length - 1; i > index; --i) {
            if (persistent) {
                node.copy_element(i + 1, this->self(), i);
            } else {
                node.move_element(i + 1, this->self(), i);
            }
        }
        node.set_element(index, split.left);
        node.set_element(index + 1, split.right);
        if (persistent) {
            for (IndexType i = 0; i < index; ++i) {
                node.copy_element(i, this->self(), i);
            }
        }
        node.length = length + 1;
    }

    template <typename F>
    static void set_element2(InternalNodeBase& left, InternalNodeBase& right, IndexType index, IndexType split_point, F&& f) {
        if (index < split_point) {
            f(left, index);
        } else {
            f(right, index - split_point);
        }
    }

    void set_element(InternalNodeBase& left, InternalNodeBase& right, IndexType index, IndexType split_point, InfoType<ChildType>& update) {
        set_element2(left, right, index, split_point,
                    [&update](InternalNodeBase& node, IndexType dest_index)
                    { node.set_element(dest_index, update); }
        );
    }

    void copy_element(InternalNodeBase& left, InternalNodeBase& right, IndexType dest_index, NodeType& source, IndexType source_index, IndexType split_point) {
        set_element2(left, right, dest_index, split_point,
                    [&source, source_index](InternalNodeBase& node, IndexType dest_index)
                    { node.copy_element(dest_index, source, source_index); }
        );
    }

    void move_element(InternalNodeBase& left, InternalNodeBase& right, IndexType dest_index, NodeType& source, IndexType source_index, IndexType split_point) {
        set_element2(left, right, dest_index, split_point,
                    [&source, source_index](InternalNodeBase& node, IndexType dest_index)
                    { node.move_element(dest_index, source, source_index); }
        );
    }

    bool insert_split_split(InternalNodeBase& left, InternalNodeBase& right, IndexType index, SplitType<ChildType>& split, uint64_t& iter, bool right_most) noexcept(disable_exceptions) {
        IndexType split_point = right_most && index + 1 == internal_size ? index + 1 : (internal_size + 1) / 2;
        for (IndexType i = length - 1; i > index; --i) {
            if (persistent) {
                copy_element(left, right, i + 1, this->self(), i, split_point);
            } else {
                move_element(left, right, i + 1, this->self(), i, split_point);
            }
        }
        if (!split.left.ptr_changed) {
            split.left.ptr = std::move(pointers[index]);
            split.left.ptr_changed = true;
        }
        set_element(left, right, index, split_point, split.left);
        set_element(left, right, index + 1, split_point, split.right);
        for (IndexType i = persistent ? 0 : split_point; i < index; ++i) {
            if (persistent) {
                copy_element(left, right, i, this->self(), i, split_point);
            } else {
                move_element(left, right, i, this->self(), i, split_point);
            }
        }
        for (IndexType i = split_point; i < left.length; ++i) {
            left.erase_element(i);
        }
        left.length = static_cast<uint16_t>(split_point);
        right.length = static_cast<uint16_t>(internal_size + 1 - split_point);
        IndexType insertion_index = split.new_element_left ? index : index + 1;
        if (insertion_index >= split_point) {
            set_index(iter, insertion_index - split_point);
            return false;
        }
        set_index(iter, insertion_index);
        return true;
    }

    template <typename R, typename S>
    void insert_split(
            IndexType index,
            SplitType<ChildType>& split,
            R&& do_replace,
            S&& do_split,
            uint64_t& iter,
            bool right_most
    ) noexcept(disable_exceptions) {
        if (length != internal_size) {
            set_index(iter, split.new_element_left ? index : index + 1);
            ReplaceType<NodeType> replace{};
            compute_delta_split(split, replace.delta, index);
            if (persistent) {
                replace.delta.ptr = make_ptr<NodeType>();
                insert_split_replace(*replace.delta.ptr, index, split);
                replace.delta.ptr_changed = true;
                do_replace(replace);
            } else {
                insert_split_replace(this->self(), index, split);
                do_replace(replace);
            }
        } else {
            auto right = make_ptr<NodeType>();
            if (persistent) {
                auto left = make_ptr<NodeType>();
                bool new_element_left = insert_split_split(*left, *right, index, split, iter, right_most);
                do_split(SplitType<NodeType>(std::move(left), std::move(right), true, new_element_left));
            } else {
                bool new_element_left = insert_split_split(this->self(), *right, index, split, iter, right_most);
                do_split(SplitType<NodeType>(&(this->self()), std::move(right), false, new_element_left));
            }
        }
    }

    template <typename T, typename F, typename R, typename S, typename... Args>
    void insert(T const& search_val, F&& finder, R&& do_replace, S&& do_split, size_t& size, uint64_t& iter, bool right_most, Args&&... args) {
        auto [index, remainder] = finder(this->self(), search_val);
        pointers[index]->insert(remainder,
                                finder,
                                [this, index = index, &do_replace, &iter](auto&& replace)
                                { this->insert_replace(index, replace, do_replace, iter); },
                                [this, index = index, &do_replace, &do_split, &iter, right_most](auto&& split)
                                { this->insert_split(index, split, do_replace, do_split, iter, right_most); },
                                size,
                                iter,
                                right_most && index == length - 1,
                                std::forward<Args>(args)...);
    }

    template <typename T, typename F, typename R, typename... Args>
    void assign(T const& search_val, F&& finder, R&& do_replace, uint64_t& iter, Args&&... args) {
        auto [index, remainder] = finder(this->self(), search_val);
        pointers[index]->assign(remainder,
                 finder,
                 [this, index = index, &do_replace, &iter](auto&& replace)
                 { this->insert_replace(index, replace, do_replace, iter); },
                 iter,
                 std::forward<Args>(args)...
        );
    }

    bool erase(InternalNodeBase& node, IndexType index, uint64_t& iter, bool right_most) noexcept(disable_exceptions) {
        if (persistent) {
            for (IndexType i = 0; i < index; ++i) {
                node.copy_element(i, this->self(), i);
            }
        }
        for (IndexType i = index + 1; i < length; ++i) {
            if (persistent) {
                node.copy_element(i - 1, this->self(), i);
            } else {
                node.move_element(i - 1, this->self(), i);
            }
        }
        if (node.length == length) {
            node.erase_element(length - 1);
        }
        node.length = length - 1;
        bool carry = index == node.length;
        if (carry && right_most) {
            node.seek_end(iter);
            return false;
        }
        set_index(iter, carry ? 0 : index);
        return carry;
    }

    template <typename R, typename E>
    void erase(IndexType index, R&& do_replace, E&& do_erase, uint64_t& iter, bool right_most) noexcept(disable_exceptions) {
        if (length > 1) {
            ReplaceType<NodeType> replace{};
            compute_delta_erase(index, replace.delta);
            if (persistent) {
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
        pointers[index]->erase(remainder,
                               finder,
                               [this, index = index, &do_replace, &iter](auto&& replace)
                               { this->insert_replace(index, replace, do_replace, iter); },
                               [this, index = index, &do_replace, &do_erase, &iter, right_most]()
                               { this->erase(index, do_replace, do_erase, iter, right_most); },
                               size,
                               iter,
                               right_most && index == length - 1);
    }

    template <typename T, typename F, typename R, typename U>
    void update(T const& search_val, F&& finder, R&& do_replace, uint64_t& iter, U&& updater) {
        auto [index, remainder] = finder(this->self(), search_val);
        pointers[index]->update(remainder,
                                finder,
                                [this, index = index, &do_replace, &iter](auto&& replace)
                                { this->insert_replace(index, replace, do_replace, iter); },
                                iter,
                                updater
        );
    }

    template <typename T, typename F, typename R, typename U>
    void update2(T const& search_val, F&& finder, R&& do_replace, uint64_t& iter, U&& updater) {
        auto [index, remainder] = finder(this->self(), search_val);
        pointers[index]->update2(remainder,
                                finder,
                                [this, index = index, &do_replace, &iter](auto&& replace)
                                { this->insert_replace(index, replace, do_replace, iter); },
                                iter,
                                updater
        );
    }

    Value const& get_iter(uint64_t it) const {
        return pointers[get_index(it)]->get_iter(it);
    }

    void make_persistent() {
        if (!persistent) {
            persistent = true;
            for (IndexType i = 0; i < length; ++i) {
                pointers[i]->make_persistent();
            }
        }
    }

    void seek_first(uint64_t& it) const {
        clear_index(it);
        pointers[0]->seek_first(it);
    }

    void seek_last(uint64_t& it) const {
        set_index(it, length - 1);
        pointers[length - 1]->seek_last(it);
    }

    void seek_end(uint64_t& it) const {
        set_index(it, length - 1);
        pointers[length - 1]->seek_end(it);
    }

    void seek_begin(typename InternalNodeBase::template LeafNodeType<leaf_size> const*& leaf, uint64_t& it) const {
        clear_index(it);
        pointers[0]->seek_begin(leaf, it);
    }

    void seek_end(typename InternalNodeBase::template LeafNodeType<leaf_size> const*& leaf, uint64_t& it) const {
        set_index(it, length - 1);
        pointers[length - 1]->seek_end(leaf, it);
    }

    Value const& front() const {
        return pointers[0]->front();
    }

    Value const& back() const {
        return pointers[length - 1]->back();
    }

    ssize advance(typename InternalNodeBase::template LeafNodeType<leaf_size> const*& leaf, uint64_t& it, ssize n) const {
        while (true) {
            n = pointers[get_index(it)]->advance(leaf, it, n);
            if (n > 0) {
                if (get_index(it) < length - 1) {
                    inc_index(it);
                    pointers[get_index(it)]->seek_first(it);
                    --n;
                    continue;
                }
                return n;
            }
            if (n < 0) {
                if (get_index(it) > 0) {
                    dec_index(it);
                    pointers[get_index(it)]->seek_last(it);
                    ++n;
                    continue;
                }
                return n;
            }
            return 0;
        }
    }
};
} //end namespace bpptree::detail
