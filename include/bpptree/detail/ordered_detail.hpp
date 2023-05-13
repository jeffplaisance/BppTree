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
#include <algorithm>
#include "helpers.hpp"
#include "uninitialized_array.hpp"

namespace bpptree::detail {

template <typename KeyValue, typename KeyValueExtractor, typename LessThan, bool binary_search>
struct OrderedDetail {
private:
    static constexpr KeyValueExtractor extractor{};

    static constexpr LessThan less_than{};

    using Key = std::remove_cv_t<std::remove_reference_t<decltype(extractor.get_key(std::declval<KeyValue const&>()))>>;
public:
    template <typename Parent>
    struct LeafNode : public Parent {

        using InfoType = typename Parent::InfoType;

        template <typename Comp>
        IndexType find_key_index(Key const& search_val, Comp const& comp) const {
            if constexpr (!binary_search) {
                IndexType index = 0;
                while (index < this->length) {
                    if (comp(extractor.get_key(this->values[index]), search_val)) {
                        ++index;
                    } else {
                        break;
                    }
                }
                return index;
            } else {
                auto it = std::lower_bound(this->values.cbegin(), this->values.cbegin() + this->length, search_val,
                                           [&comp](auto const& a, auto const& b) { return comp(extractor.get_key(a), b); });
                return static_cast<IndexType>(std::distance(this->values.cbegin(), it));
            }
        }

        IndexType lower_bound_index(Key const& search_val) const {
            return find_key_index(search_val, [](auto const& a, auto const& b) { return less_than(a, b); });
        }

        IndexType find_key_index_checked(Key const& search_val) const {
            IndexType ret = lower_bound_index(search_val);
#ifdef BPPTREE_SAFETY_CHECKS
            if (ret >= this->length) {
                throw std::logic_error("key not found!");
            }
            decltype(auto) extracted = extractor.get_key(this->values[ret]);
            if (less_than(extracted, search_val) || less_than(search_val, extracted)) {
                throw std::logic_error("key not found!");
            }
#endif
            return ret;
        }

        template <typename... Args>
        void compute_delta_insert2(IndexType index, InfoType& node_info, Args const& ... args) const {
            if (index == this->length) {
                node_info.key = extractor.get_key(args...);
            } else {
                node_info.key = last_key();
            }
            Parent::compute_delta_insert2(index, node_info, args...);
        }

        template <typename... Args>
        void compute_delta_set2(IndexType index, InfoType& node_info, Args const& ... args) const {
            if (index == this->length - 1) {
                node_info.key = extractor.get_key(args...);
            } else {
                node_info.key = last_key();
            }
            Parent::compute_delta_set2(index, node_info, args...);
        }

        void compute_delta_erase2(IndexType index, InfoType& node_info) const {
            if (index == this->length - 1) {
                node_info.key = extractor.get_key(this->values[this->length - 2]);
            } else {
                node_info.key = last_key();
            }
            Parent::compute_delta_erase2(index, node_info);
        }

        template <DuplicatePolicy duplicate_policy, typename F, typename R, typename S, typename... Args>
        void insert_or_assign(Key const& search_val, F&& finder, R&& do_replace, S&& do_split,
                            size_t& size, uint64_t& iter, bool right_most, Args&& ... args) {
            auto [index, remainder] = finder(this->self(), search_val);

            if constexpr (duplicate_policy != DuplicatePolicy::insert) {
                if (index < this->length) {
                    decltype(auto) extracted = extractor.get_key(std::as_const(this->values[index]));
                    if (!less_than(extracted, search_val) && !less_than(search_val, extracted)) {
                        if constexpr (duplicate_policy == DuplicatePolicy::replace) {
                            this->self().assign2(index, do_replace, iter, std::forward<Args>(args)...);
                        }
                        return;
                    }
                }
            }
            this->self().insert_index(index, do_replace, do_split, size, iter, right_most, std::forward<Args>(args)...);
        }

        decltype(auto) first_key() const {
            return extractor.get_key(this->values[0]);
        }

        decltype(auto) last_key() const {
            return extractor.get_key(this->values[this->length - 1]);
        }

        decltype(auto) at_key(Key const& search_val) const {
            IndexType index = find_key_index_checked(search_val);
            return extractor.get_value(this->values[index]);
        }

        template <typename Comp>
        bool seek_key(
                typename Parent::SelfType const*& leaf,
                uint64_t& it,
                Key const& key,
                Comp const& comp
        ) const {
            IndexType index = find_key_index(key, comp);
            this->set_index(it, index);
            leaf = &this->self();
            if (index < this->length) {
                decltype(auto) extracted = extractor.get_key(this->values[index]);
                return !less_than(extracted, key) && !less_than(key, extracted);
            }
            return false;
        }

        bool contains(Key const& key) const {
            IndexType index = lower_bound_index(key);
            if (index < this->length) {
                decltype(auto) extracted = extractor.get_key(this->values[index]);
                return !less_than(extracted, key) && !less_than(key, extracted);
            }
            return false;
        }
    };

    template <typename Parent, auto internal_size>
    struct InternalNode : public Parent {

        using NodeType = typename Parent::NodeType;

        using ChildType = typename Parent::ChildType;

        template <typename PtrType>
        using InfoType = typename Parent::template InfoType<PtrType>;

        template <typename PtrType>
        using SplitType = typename Parent::template SplitType<PtrType>;

        UninitializedArray<Key, internal_size> keys;

        InternalNode() = default;

        InternalNode(InternalNode const& other) noexcept(noexcept(Parent(other)) && std::is_nothrow_copy_constructible_v<Key>) : Parent(other), keys(other.keys, other.length) {}

        ~InternalNode() {
            if constexpr (!std::is_trivially_destructible_v<Key>) {
                for (IndexType i = 0; i < this->length; ++i) {
                    keys.destruct(i);
                }
            }
        }

        void erase_element2(IndexType index) {
            keys.destruct(index);
            Parent::erase_element2(index);
        }

        void move_element2(IndexType dest_index, NodeType& source, IndexType source_index) {
            keys.set(dest_index, this->length, source.keys.move(source_index));
            Parent::move_element2(dest_index, source, source_index);
        }

        void copy_element2(IndexType dest_index, NodeType const& source, IndexType source_index) {
            keys.set(dest_index, this->length, source.keys[source_index]);
            Parent::copy_element2(dest_index, source, source_index);
        }

        void replace_element2(IndexType index, InfoType<ChildType>& t) {
            keys[index] = std::move(t.key);
            Parent::replace_element2(index, t);
        }

        void set_element2(IndexType index, InfoType<ChildType>& t) {
            keys.set(index, this->length, std::move(t.key));
            Parent::set_element2(index, t);
        }

        void
        compute_delta_split2(SplitType<ChildType> const& split, InfoType<NodeType>& node_info, IndexType index) const {
            if (index == this->length - 1) {
                node_info.key = split.right.key;
            } else {
                node_info.key = last_key();
            }
            Parent::compute_delta_split2(split, node_info, index);
        }

        void
        compute_delta_replace2(InfoType<ChildType> const& update, InfoType<NodeType>& node_info, IndexType index) const {
            if (index == this->length - 1) {
                node_info.key = update.key;
            } else {
                node_info.key = last_key();
            }
            Parent::compute_delta_replace2(update, node_info, index);
        }

        void compute_delta_erase2(IndexType index, InfoType<NodeType>& node_info) const {
            if (index == this->length - 1) {
                node_info.key = this->keys[this->length - 2];
            } else {
                node_info.key = last_key();
            }
            Parent::compute_delta_erase2(index, node_info);
        }

        template <typename Comp>
        IndexType find_key_index(Key const& search_val, Comp const& comp) const {
            if constexpr (!binary_search) {
                IndexType index = 0;
                while (index < this->length - 1) {
                    if (comp(keys[index], search_val)) {
                        ++index;
                    } else {
                        break;
                    }
                }
                return index;
            } else {
                if (this->length == 0) {
                    return 0;
                }
                auto it = std::lower_bound(keys.cbegin(), keys.cbegin() + (this->length - 1), search_val, comp);
                return static_cast<IndexType>(std::distance(keys.cbegin(), it));
            }
        }

        IndexType lower_bound_index(Key const& search_val) const {
            return find_key_index(search_val, [](auto const& a, auto const& b) { return less_than(a, b); });
        }

        IndexType find_key_index_checked(Key const& search_val) const {
            return lower_bound_index(search_val);
        }

        template <DuplicatePolicy duplicate_policy, typename F, typename R, typename S, typename... Args>
        void insert_or_assign(Key const& search_val, F&& finder, R&& do_replace, S&& do_split,
                            size_t& size, uint64_t& iter, bool right_most, Args&& ... args) {
            auto [index, remainder] = finder(this->self(), search_val);
            this->pointers[index]->template insert_or_assign<duplicate_policy>(remainder,
                                                  finder,
                                                  [this, index = index, &do_replace, &iter](auto&& replace) {
                                                      this->insert_replace(index, replace, do_replace, iter);
                                                  },
                                                  [this, index = index, &do_replace, &do_split, right_most, &iter](auto&& split) {
                                                      this->insert_split(index, split, do_replace, do_split, iter, right_most);
                                                  },
                                                  size,
                                                  iter,
                                                  right_most && index == this->length - 1,
                                                  std::forward<Args>(args)...);
        }

        decltype(auto) at_key(Key const& search_val) const {
            return this->pointers[lower_bound_index(search_val)]->at_key(search_val);
        }

        template <typename L, typename Comp>
        bool seek_key(
                L const*& leaf,
                uint64_t& it,
                Key const& key,
                Comp const& comp
        ) const {
            IndexType index = find_key_index(key, comp);
            this->set_index(it, index);
            return this->pointers[index]->seek_key(leaf, it, key, comp);
        }

        bool contains(Key const& key) const {
            return this->pointers[lower_bound_index(key)]->contains(key);
        }

        decltype(auto) first_key() const {
            return this->pointers[0]->first_key();
        }

        decltype(auto) last_key() const {
            return this->pointers[this->length - 1]->last_key();
        }
    };

    template <typename Parent>
    struct NodeInfo : public Parent {
        Key key{};

        NodeInfo() = default;

        template <typename P>
        NodeInfo(P const& p, const bool changed) : Parent(p, changed), key(p->last_key()) {}
    };
};
} //end namespace bpptree::detail
