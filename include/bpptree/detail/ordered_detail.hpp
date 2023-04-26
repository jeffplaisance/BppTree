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

template <typename KeyValue, typename KeyValueExtractor, typename LessThan, bool binarySearch>
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
        IndexType findKeyIndex(Key const& searchVal, Comp const& comp) const {
            if constexpr (!binarySearch) {
                IndexType index = 0;
                while (index < this->length) {
                    if (comp(extractor.get_key(this->values[index]), searchVal)) {
                        ++index;
                    } else {
                        break;
                    }
                }
                return index;
            } else {
                auto it = std::lower_bound(this->values.cbegin(), this->values.cbegin() + this->length, searchVal,
                                           [&comp](auto const& a, auto const& b) { return comp(extractor.get_key(a), b); });
                return static_cast<IndexType>(std::distance(this->values.cbegin(), it));
            }
        }

        IndexType lowerBoundIndex(Key const& searchVal) const {
            return findKeyIndex(searchVal, [](auto const& a, auto const& b) { return less_than(a, b); });
        }

        IndexType findKeyIndexChecked(Key const& searchVal) const {
            IndexType ret = lowerBoundIndex(searchVal);
            if (ret >= this->length) {
                throw std::logic_error("key not found!");
            }
            decltype(auto) extracted = extractor.get_key(this->values[ret]);
            if (less_than(extracted, searchVal) || less_than(searchVal, extracted)) {
                throw std::logic_error("key not found!");
            }
            return ret;
        }

        template <typename... Args>
        void computeDeltaInsert2(IndexType index, InfoType& nodeInfo, Args const& ... args) const {
            if (index == this->length) {
                nodeInfo.key = extractor.get_key(args...);
            } else {
                nodeInfo.key = lastKey();
            }
            Parent::computeDeltaInsert2(index, nodeInfo, args...);
        }

        template <typename... Args>
        void computeDeltaSet2(IndexType index, InfoType& nodeInfo, Args const& ... args) const {
            if (index == this->length - 1) {
                nodeInfo.key = extractor.get_key(args...);
            } else {
                nodeInfo.key = lastKey();
            }
            Parent::computeDeltaSet2(index, nodeInfo, args...);
        }

        void computeDeltaErase2(IndexType index, InfoType& nodeInfo) const {
            if (index == this->length - 1) {
                nodeInfo.key = extractor.get_key(this->values[this->length - 2]);
            } else {
                nodeInfo.key = lastKey();
            }
            Parent::computeDeltaErase2(index, nodeInfo);
        }

        template <typename F, typename R, typename S, typename Policy, typename... Args>
        void insertOrAssign(Key const& searchVal, F&& finder, R&& doReplace, S&& doSplit,
                            size_t& size, uint64_t& iter, bool rightMost, Policy const&, Args&& ... args) {
            auto [index, remainder] = finder(this->self(), searchVal);

            if constexpr (Policy::value != DuplicatePolicy::insert) {
                if (index < this->length) {
                    decltype(auto) extracted = extractor.get_key(std::as_const(this->values[index]));
                    if (!less_than(extracted, searchVal) && !less_than(searchVal, extracted)) {
                        if constexpr (Policy::value == DuplicatePolicy::replace) {
                            this->self().assign2(index, doReplace, iter, std::forward<Args>(args)...);
                        }
                        return;
                    }
                }
            }
            this->self().insertIndex(index, doReplace, doSplit, size, iter, rightMost, std::forward<Args>(args)...);
        }

        decltype(auto) firstKey() const {
            return extractor.get_key(this->values[0]);
        }

        decltype(auto) lastKey() const {
            return extractor.get_key(this->values[this->length - 1]);
        }

        decltype(auto) at_key(Key const& searchVal) const {
            IndexType index = findKeyIndexChecked(searchVal);
            return extractor.get_value(this->values[index]);
        }

        template <typename Comp>
        bool seekKey(
                typename Parent::SelfType const*& leaf,
                uint64_t& it,
                Key const& key,
                Comp const& comp
        ) const {
            IndexType index = findKeyIndex(key, comp);
            this->setIndex(it, index);
            leaf = &this->self();
            if (index < this->length) {
                decltype(auto) extracted = extractor.get_key(this->values[index]);
                return !less_than(extracted, key) && !less_than(key, extracted);
            }
            return false;
        }

        bool contains(Key const& key) const {
            IndexType index = lowerBoundIndex(key);
            if (index < this->length) {
                decltype(auto) extracted = extractor.get_key(this->values[index]);
                return !less_than(extracted, key) && !less_than(key, extracted);
            }
            return false;
        }
    };

    template <typename Parent, auto InternalSize>
    struct InternalNode : public Parent {

        using NodeType = typename Parent::NodeType;

        using ChildType = typename Parent::ChildType;

        template <typename PtrType>
        using InfoType = typename Parent::template InfoType<PtrType>;

        template <typename PtrType>
        using SplitType = typename Parent::template SplitType<PtrType>;

        UninitializedArray<Key, InternalSize> keys;

        InternalNode() noexcept = default;

        InternalNode(InternalNode const& other) noexcept: Parent(other), keys(other.keys, other.length) {}

        ~InternalNode() {
            if constexpr (!std::is_trivially_destructible_v<Key>) {
                for (IndexType i = 0; i < this->length; ++i) {
                    keys.destruct(i);
                }
            }
        }

        void eraseElement2(IndexType index) {
            keys.destruct(index);
            Parent::eraseElement2(index);
        }

        void moveElement2(IndexType destIndex, NodeType& source, IndexType sourceIndex) {
            keys.set(destIndex, this->length, source.keys.move(sourceIndex));
            Parent::moveElement2(destIndex, source, sourceIndex);
        }

        void copyElement2(IndexType destIndex, NodeType const& source, IndexType sourceIndex) {
            keys.set(destIndex, this->length, source.keys[sourceIndex]);
            Parent::copyElement2(destIndex, source, sourceIndex);
        }

        void replaceElement2(IndexType index, InfoType<ChildType>& t) {
            keys[index] = std::move(t.key);
            Parent::replaceElement2(index, t);
        }

        void setElement2(IndexType index, InfoType<ChildType>& t) {
            keys.set(index, this->length, std::move(t.key));
            Parent::setElement2(index, t);
        }

        void
        computeDeltaSplit2(SplitType<ChildType> const& split, InfoType<NodeType>& nodeInfo, IndexType index) const {
            if (index == this->length - 1) {
                nodeInfo.key = split.right.key;
            } else {
                nodeInfo.key = lastKey();
            }
            Parent::computeDeltaSplit2(split, nodeInfo, index);
        }

        void
        computeDeltaReplace2(InfoType<ChildType> const& update, InfoType<NodeType>& nodeInfo, IndexType index) const {
            if (index == this->length - 1) {
                nodeInfo.key = update.key;
            } else {
                nodeInfo.key = lastKey();
            }
            Parent::computeDeltaReplace2(update, nodeInfo, index);
        }

        void computeDeltaErase2(IndexType index, InfoType<NodeType>& nodeInfo) const {
            if (index == this->length - 1) {
                nodeInfo.key = this->keys[this->length - 2];
            } else {
                nodeInfo.key = lastKey();
            }
            Parent::computeDeltaErase2(index, nodeInfo);
        }

        template <typename Comp>
        IndexType findKeyIndex(Key const& searchVal, Comp const& comp) const {
            if constexpr (!binarySearch) {
                IndexType index = 0;
                while (index < this->length - 1) {
                    if (comp(keys[index], searchVal)) {
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
                auto it = std::lower_bound(keys.cbegin(), keys.cbegin() + (this->length - 1), searchVal, comp);
                return static_cast<IndexType>(std::distance(keys.cbegin(), it));
            }
        }

        IndexType lowerBoundIndex(Key const& searchVal) const {
            return findKeyIndex(searchVal, [](auto const& a, auto const& b) { return less_than(a, b); });
        }

        IndexType findKeyIndexChecked(Key const& searchVal) const {
            return lowerBoundIndex(searchVal);
        }

        template <typename F, typename R, typename S, typename P, typename... Args>
        void insertOrAssign(Key const& searchVal, F&& finder, R&& doReplace, S&& doSplit,
                            size_t& size, uint64_t& iter, bool rightMost, P const& duplicate_policy, Args&& ... args) {
            auto [index, remainder] = finder(this->self(), searchVal);
            this->pointers[index]->insertOrAssign(remainder,
                                                  finder,
                                                  [this, index = index, &doReplace, &iter](auto&& replace) {
                                                      this->insertReplace(index, replace, doReplace, iter);
                                                  },
                                                  [this, index = index, &doReplace, &doSplit, rightMost, &iter](auto&& split) {
                                                      this->insertSplit(index, split, doReplace, doSplit, iter, rightMost);
                                                  },
                                                  size,
                                                  iter,
                                                  rightMost && index == this->length - 1,
                                                  duplicate_policy,
                                                  std::forward<Args>(args)...);
        }

        decltype(auto) at_key(Key const& searchVal) const {
            return this->pointers[lowerBoundIndex(searchVal)]->at_key(searchVal);
        }

        template <typename L, typename Comp>
        bool seekKey(
                L const*& leaf,
                uint64_t& it,
                Key const& key,
                Comp const& comp
        ) const {
            IndexType index = findKeyIndex(key, comp);
            this->setIndex(it, index);
            return this->pointers[index]->seekKey(leaf, it, key, comp);
        }

        bool contains(Key const& key) const {
            return this->pointers[lowerBoundIndex(key)]->contains(key);
        }

        decltype(auto) firstKey() const {
            return this->pointers[0]->firstKey();
        }

        decltype(auto) lastKey() const {
            return this->pointers[this->length - 1]->lastKey();
        }
    };

    template <typename Parent>
    struct NodeInfo : public Parent {
        Key key{};

        NodeInfo() noexcept = default;

        template <typename P>
        NodeInfo(P&& p, const bool changed) noexcept : Parent(std::forward<P>(p), changed),
                                                              key(changed ? this->ptr->lastKey() : p->lastKey()) {}
    };
};
} //end namespace bpptree::detail
