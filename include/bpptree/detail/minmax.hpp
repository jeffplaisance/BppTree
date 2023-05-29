//
// B++ Tree: A B+ Tree library written in C++
// Copyright (C) 2023 Jeff Plaisance
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#include <type_traits>
#include <algorithm>
#include "helpers.hpp"

#define BPPTREE_CONCAT_INNER(a, b) a##b
#define BPPTREE_CONCAT(a, b) BPPTREE_CONCAT_INNER(a, b)
#define BPPTREE_MINMAXS BPPTREE_CONCAT(BPPTREE_MINMAX, s)

namespace bpptree::detail {

template <typename Value, typename KeyExtractor, typename Comp>
struct BPPTREE_CONCAT(BPPTREE_MINMAX_UPPER, Detail) {

    static constexpr Comp comp{};

    static constexpr KeyExtractor extractor{};

    using KeyRef = decltype(extractor(std::declval<Value>()));

    using Key = std::remove_cv_t<std::remove_reference_t<KeyRef>>;

    template <typename Parent>
    struct LeafNode : public Parent {

        using InfoType = typename Parent::InfoType;

        template <typename T = Empty, bool enable_exclude = !std::is_same_v<T, Empty>>
        IndexType BPPTREE_CONCAT(BPPTREE_MINMAX, _excluding)(IndexType begin, IndexType end, T exclude_index = Empty::empty) const {
            IndexType min_max_index;
            IndexType start;
            if constexpr (enable_exclude) {
                start = exclude_index == begin ? begin + 1 : begin;
            } else {
                start = begin;
            }
            if (start > end) {
                min_max_index = -1;
            } else {
                min_max_index = start;
                for (IndexType i = start + 1; i <= end; ++i) {
                    if constexpr (enable_exclude) {
                        if (i == exclude_index) {
                            continue;
                        }
                    }
                    if (comp(extractor(this->values[i]), extractor(this->values[min_max_index]))) {
                        min_max_index = i;
                    }
                }
            }
            return min_max_index;
        }

        template <typename T = Empty>
        auto BPPTREE_CONCAT(BPPTREE_MINMAX, _excluding)(T exclude_index = Empty::empty) const {
            return BPPTREE_CONCAT(BPPTREE_MINMAX, _excluding)(0, this->length - 1, exclude_index);
        }

        template <typename T, typename... Args>
        void compute_delta(T index, InfoType& node_info, Args const&... args) const {
            IndexType min_max_index = BPPTREE_CONCAT(BPPTREE_MINMAX, _excluding)(index);
            if (min_max_index >= 0) {
                decltype(auto) a = extractor(args...);
                decltype(auto) b = extractor(this->values[min_max_index]);
                node_info.BPPTREE_MINMAX = comp(a, b) ? a : b;
            } else {
                node_info.BPPTREE_MINMAX = extractor(args...);
            }
        }

        template <typename... Args>
        void compute_delta_insert2(IndexType index, InfoType& node_info, Args const&... args) const {
            compute_delta(Empty::empty, node_info, args...);
            Parent::compute_delta_insert2(index, node_info, args...);
        }

        template <typename... Args>
        void compute_delta_set2(IndexType index, InfoType& node_info, Args const&... args) const {
            compute_delta(index, node_info, args...);
            Parent::compute_delta_set2(index, node_info, args...);
        }

        void compute_delta_erase2(IndexType index, InfoType& node_info) const {
            IndexType min_max_index = BPPTREE_CONCAT(BPPTREE_MINMAX, _excluding)(index);
            if (min_max_index >= 0) {
                node_info.BPPTREE_MINMAX = extractor(this->values[min_max_index]);
            }
            Parent::compute_delta_erase2(index, node_info);
        }

        [[nodiscard]] KeyRef BPPTREE_MINMAX() const {
#ifdef BPPTREE_SAFETY_CHECKS
            if (this->length == 0) {
                throw std::out_of_range("cannot call " BPPTREE_MINMAXSTR " on empty node");
            }
#endif
            IndexType min_max_index = BPPTREE_CONCAT(BPPTREE_MINMAX, _excluding)();
            return extractor(this->values[min_max_index]);
        }

        [[nodiscard]] KeyRef BPPTREE_MINMAX(uint64_t begin, uint64_t end) const {
            IndexType begin_index = this->get_index(begin);
            IndexType end_index = this->get_index(end);
#ifdef BPPTREE_SAFETY_CHECKS
            if (begin_index > end_index) {
                throw std::out_of_range("cannot call " BPPTREE_MINMAXSTR " on empty range");
            }
#endif
            IndexType min_max_index = BPPTREE_CONCAT(BPPTREE_MINMAX, _excluding)(begin_index, end_index);
            return extractor(this->values[min_max_index]);
        }

        [[nodiscard]] KeyRef BPPTREE_CONCAT(BPPTREE_MINMAX, _after)(uint64_t begin) const {
            return BPPTREE_MINMAX(begin, this->length - 1);
        }

        [[nodiscard]] KeyRef BPPTREE_CONCAT(BPPTREE_MINMAX, _before)(uint64_t end) const {
            return BPPTREE_MINMAX(0, end);
        }

        template <typename It>
        void BPPTREE_MINMAX(It& it) const {
#ifdef BPPTREE_SAFETY_CHECKS
            if (this->length == 0) {
                throw std::out_of_range("cannot call " BPPTREE_MINMAXSTR " on empty node");
            }
#endif
            IndexType min_max_index = BPPTREE_CONCAT(BPPTREE_MINMAX, _excluding)();
            this->set_index(it.iter, min_max_index);
            it.leaf = &this->self();
        }

        template <typename It>
        void BPPTREE_MINMAX(It& it, uint64_t begin, uint64_t end) const {
            IndexType begin_index = this->get_index(begin);
            IndexType end_index = this->get_index(end);
#ifdef BPPTREE_SAFETY_CHECKS
            if (begin_index > end_index) {
                throw std::out_of_range("cannot call " BPPTREE_MINMAXSTR " on empty range");
            }
#endif
            IndexType min_max_index = BPPTREE_CONCAT(BPPTREE_MINMAX, _excluding)(begin_index, end_index);
            this->set_index(it.iter, min_max_index);
            it.leaf = &this->self();
        }

        template <typename It>
        void BPPTREE_CONCAT(BPPTREE_MINMAX, _after)(It& it, uint64_t begin) const {
            BPPTREE_MINMAX(it, begin, this->length - 1);
        }

        template <typename It>
        void BPPTREE_CONCAT(BPPTREE_MINMAX, _before)(It& it, uint64_t end) const {
            BPPTREE_MINMAX(it, 0, end);
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

        UninitializedArray<Key, internal_size> BPPTREE_MINMAXS;

        InternalNode() = default;

        InternalNode(InternalNode const& other) noexcept(noexcept(Parent(other)) && std::is_nothrow_copy_constructible_v<Key>) : Parent(other), BPPTREE_MINMAXS(other.BPPTREE_MINMAXS, other.length) {}

        InternalNode& operator=(InternalNode const& other) = delete;

        ~InternalNode() {
            for (IndexType i = 0; i < this->length; ++i) {
                BPPTREE_MINMAXS.destruct(i);
            }
        }

        void erase_element2(IndexType index) {
            BPPTREE_MINMAXS.destruct(index);
            Parent::erase_element2(index);
        }

        void move_element2(IndexType dest_index, NodeType& source, IndexType source_index) {
            BPPTREE_MINMAXS.set(dest_index, this->length, source.BPPTREE_MINMAXS.move(source_index));
            Parent::move_element2(dest_index, source, source_index);
        }

        void copy_element2(IndexType dest_index, NodeType const& source, IndexType source_index) {
            BPPTREE_MINMAXS.set(dest_index, this->length, source.BPPTREE_MINMAXS[source_index]);
            Parent::copy_element2(dest_index, source, source_index);
        }

        void replace_element2(IndexType index, InfoType<ChildType>& t) {
            BPPTREE_MINMAXS[index] = std::move(t.BPPTREE_MINMAX);
            Parent::replace_element2(index, t);
        }

        void set_element2(IndexType index, InfoType<ChildType>& t) {
            BPPTREE_MINMAXS.set(index, this->length, std::move(t.BPPTREE_MINMAX));
            Parent::set_element2(index, t);
        }

        template <typename T = Empty, bool enable_exclude = !std::is_same_v<T, Empty>>
        auto BPPTREE_CONCAT(BPPTREE_MINMAX, _excluding)(IndexType begin, IndexType end, T exclude_index = Empty::empty) const {
            std::tuple<Key const*, IndexType> ret;
            auto& [BPPTREE_MINMAX, min_max_index] = ret;
            IndexType start;
            if constexpr (enable_exclude) {
                start = exclude_index == begin ? begin + 1 : begin;
            } else {
                start = begin;
            }
            if (start >= end) {
                BPPTREE_MINMAX = nullptr;
                min_max_index = -1;
            } else {
                BPPTREE_MINMAX = &BPPTREE_MINMAXS[start];
                min_max_index = start;
                for (IndexType i = start + 1; i < end; ++i) {
                    if constexpr (enable_exclude) {
                        if (i == exclude_index) {
                            continue;
                        }
                    }
                    if (comp(BPPTREE_MINMAXS[i], *BPPTREE_MINMAX)) {
                        BPPTREE_MINMAX = &BPPTREE_MINMAXS[i];
                        min_max_index = i;
                    }
                }
            }
            return ret;
        }

        template <typename T = Empty>
        auto BPPTREE_CONCAT(BPPTREE_MINMAX, _excluding)(T exclude_index = Empty::empty) const {
            return BPPTREE_CONCAT(BPPTREE_MINMAX, _excluding)(0, this->length, exclude_index);
        }

        void compute_delta_split2(SplitType<ChildType> const& split, InfoType<NodeType>& node_info, IndexType index) const {
            auto [BPPTREE_MINMAX, min_max_index] = BPPTREE_CONCAT(BPPTREE_MINMAX, _excluding)(index);
            if (BPPTREE_MINMAX == nullptr) {
                node_info.BPPTREE_MINMAX = std::min(split.left.BPPTREE_MINMAX, split.right.BPPTREE_MINMAX, comp);
            } else {
                if (comp(split.left.BPPTREE_MINMAX, *BPPTREE_MINMAX)) {
                    BPPTREE_MINMAX = &split.left.BPPTREE_MINMAX;
                }
                if (comp(split.right.BPPTREE_MINMAX, *BPPTREE_MINMAX)) {
                    BPPTREE_MINMAX = &split.right.BPPTREE_MINMAX;
                }
                node_info.BPPTREE_MINMAX = *BPPTREE_MINMAX;
            }
            Parent::compute_delta_split2(split, node_info, index);
        }

        void compute_delta_replace2(InfoType<ChildType> const& update, InfoType<NodeType>& node_info, IndexType index) const {
            auto [BPPTREE_MINMAX, min_max_index] = BPPTREE_CONCAT(BPPTREE_MINMAX, _excluding)(index);
            if (BPPTREE_MINMAX != nullptr && comp(*BPPTREE_MINMAX, update.BPPTREE_MINMAX)) {
                node_info.BPPTREE_MINMAX = *BPPTREE_MINMAX;
            } else {
                node_info.BPPTREE_MINMAX = update.BPPTREE_MINMAX;
            }
            Parent::compute_delta_replace2(update, node_info, index);
        }

        void compute_delta_erase2(IndexType index, InfoType<NodeType>& node_info) const {
            auto [BPPTREE_MINMAX, min_max_index] = BPPTREE_CONCAT(BPPTREE_MINMAX, _excluding)(index);
            if (BPPTREE_MINMAX != nullptr) {
                node_info.BPPTREE_MINMAX = *BPPTREE_MINMAX;
            }
            Parent::compute_delta_erase2(index, node_info);
        }

        [[nodiscard]] KeyRef BPPTREE_MINMAX() const {
#ifdef BPPTREE_SAFETY_CHECKS
            if (this->length == 0) {
                throw std::out_of_range("cannot call " BPPTREE_MINMAXSTR " on empty node");
            }
#endif
            auto [BPPTREE_MINMAX, min_max_index] = BPPTREE_CONCAT(BPPTREE_MINMAX, _excluding)();
            return *BPPTREE_MINMAX;
        }

        [[nodiscard]] Key const& BPPTREE_CONCAT(BPPTREE_MINMAX, _internal)(Key const& smallest, IndexType begin_index, IndexType end_index) const {
            auto [BPPTREE_MINMAX, min_max_index] = BPPTREE_CONCAT(BPPTREE_MINMAX, _excluding)(begin_index, end_index);
            return BPPTREE_MINMAX != nullptr && comp(*BPPTREE_MINMAX, smallest) ? *BPPTREE_MINMAX : smallest;
        }

        [[nodiscard]] KeyRef BPPTREE_CONCAT(BPPTREE_MINMAX, _after)(uint64_t begin) const {
            IndexType begin_index = this->get_index(begin);
            decltype(auto) first = this->pointers[begin_index]->BPPTREE_CONCAT(BPPTREE_MINMAX, _after)(begin);
            return BPPTREE_CONCAT(BPPTREE_MINMAX, _internal)(first, begin_index+1, this->length);
        }

        [[nodiscard]] KeyRef BPPTREE_CONCAT(BPPTREE_MINMAX, _before)(uint64_t end) const {
            IndexType end_index = this->get_index(end);
            decltype(auto) last = this->pointers[end_index]->BPPTREE_CONCAT(BPPTREE_MINMAX, _before)(end);
            return BPPTREE_CONCAT(BPPTREE_MINMAX, _internal)(last, 0, end_index);
        }

        [[nodiscard]] KeyRef BPPTREE_MINMAX(uint64_t begin, uint64_t end) const {
            IndexType begin_index = this->get_index(begin);
            IndexType end_index = this->get_index(end);
            if (begin_index == end_index) {
                return this->pointers[begin_index]->BPPTREE_MINMAX(begin, end);
            }
            decltype(auto) first = this->pointers[begin_index]->BPPTREE_CONCAT(BPPTREE_MINMAX, _after)(begin);
            decltype(auto) last = this->pointers[end_index]->BPPTREE_CONCAT(BPPTREE_MINMAX, _before)(end);
            decltype(auto) tmp = std::min(first, last, comp);
            return BPPTREE_CONCAT(BPPTREE_MINMAX, _internal)(tmp, begin_index+1, end_index);
        }

        template <typename It>
        void BPPTREE_MINMAX(It& it) const {
#ifdef BPPTREE_SAFETY_CHECKS
            if (this->length == 0) {
                throw std::out_of_range("cannot call " BPPTREE_MINMAXSTR " on empty node");
            }
#endif
            auto [BPPTREE_MINMAX, min_max_index] = BPPTREE_CONCAT(BPPTREE_MINMAX, _excluding)();
            this->set_index(it.iter, min_max_index);
            this->pointers[min_max_index]->BPPTREE_MINMAX(it);
        }

        template <typename It>
        void BPPTREE_CONCAT(BPPTREE_MINMAX, _internal)(It& it, It const& smallest, IndexType begin_index, IndexType end_index) const {
            auto [BPPTREE_MINMAX, min_max_index] = BPPTREE_CONCAT(BPPTREE_MINMAX, _excluding)(begin_index, end_index);
            if (BPPTREE_MINMAX != nullptr && comp(*BPPTREE_MINMAX, extractor(smallest.get()))) {
                this->set_index(it.iter, min_max_index);
                this->pointers[min_max_index]->BPPTREE_MINMAX(it);
            } else {
                it = smallest;
            }
        }

        template <typename It>
        void BPPTREE_CONCAT(BPPTREE_MINMAX, _after)(It& it, uint64_t begin) const {
            IndexType begin_index = this->get_index(begin);
            It b(it);
            this->set_index(b.iter, begin_index);
            this->pointers[begin_index]->BPPTREE_CONCAT(BPPTREE_MINMAX, _after)(b, begin);
            BPPTREE_CONCAT(BPPTREE_MINMAX, _internal)(it, b, begin_index + 1, this->length);
        }

        template <typename It>
        void BPPTREE_CONCAT(BPPTREE_MINMAX, _before)(It& it, uint64_t end) const {
            IndexType end_index = this->get_index(end);
            It e(it);
            this->set_index(e.iter, end_index);
            this->pointers[end_index]->BPPTREE_CONCAT(BPPTREE_MINMAX, _before)(e, end);
            BPPTREE_CONCAT(BPPTREE_MINMAX, _internal)(it, e, 0, end_index);
        }

        template <typename It>
        void BPPTREE_MINMAX(It& it, uint64_t begin, uint64_t end) const {
            IndexType begin_index = this->get_index(begin);
            IndexType end_index = this->get_index(end);
            if (begin_index == end_index) {
                this->set_index(it.iter, begin_index);
                this->pointers[begin_index]->BPPTREE_MINMAX(it, begin, end);
                return;
            }
            It b(it);
            this->set_index(b.iter, begin_index);
            this->pointers[begin_index]->BPPTREE_CONCAT(BPPTREE_MINMAX, _after)(b, begin);
            It e(it);
            this->set_index(e.iter, end_index);
            this->pointers[end_index]->BPPTREE_CONCAT(BPPTREE_MINMAX, _before)(e, end);
            It const& tmp = comp(extractor(b.get()), extractor(e.get())) ? b : e;
            BPPTREE_CONCAT(BPPTREE_MINMAX, _internal)(it, tmp, begin_index + 1, end_index);
        }
    };

    template <typename Parent>
    struct NodeInfo : public Parent {
        Key BPPTREE_MINMAX{};

        NodeInfo() = default;

        template <typename P>
        NodeInfo(P const& p, const bool changed) : Parent(p, changed), BPPTREE_MINMAX(p->BPPTREE_MINMAX()) {}
    };

    template <typename Parent>
    struct Transient : public Parent {
        template <typename... Us>
        explicit Transient(Us&&... us) : Parent(std::forward<Us>(us)...) {}
    };

    template <typename Parent>
    struct Persistent : public Parent {
        template <typename... Us>
        explicit Persistent(Us&&... us) : Parent(std::forward<Us>(us)...) {}
    };
};
}
