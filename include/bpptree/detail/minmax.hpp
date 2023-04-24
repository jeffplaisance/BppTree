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
        void computeDelta(T index, InfoType& nodeInfo, Args const&... args) const {
            IndexType min_max_index = BPPTREE_CONCAT(BPPTREE_MINMAX, _excluding)(index);
            if (min_max_index >= 0) {
                decltype(auto) a = extractor(args...);
                decltype(auto) b = extractor(this->values[min_max_index]);
                nodeInfo.BPPTREE_MINMAX = comp(a, b) ? a : b;
            } else {
                nodeInfo.BPPTREE_MINMAX = extractor(args...);
            }
        }

        template <typename... Args>
        void computeDeltaInsert2(IndexType index, InfoType& nodeInfo, Args const&... args) const {
            computeDelta(Empty::empty, nodeInfo, args...);
            Parent::computeDeltaInsert2(index, nodeInfo, args...);
        }

        template <typename... Args>
        void computeDeltaSet2(IndexType index, InfoType& nodeInfo, Args const&... args) const {
            computeDelta(index, nodeInfo, args...);
            Parent::computeDeltaSet2(index, nodeInfo, args...);
        }

        void computeDeltaErase2(IndexType index, InfoType& nodeInfo) const {
            IndexType min_max_index = BPPTREE_CONCAT(BPPTREE_MINMAX, _excluding)(index);
            if (min_max_index >= 0) {
                nodeInfo.BPPTREE_MINMAX = extractor(this->values[min_max_index]);
            }
            Parent::computeDeltaErase2(index, nodeInfo);
        }

        [[nodiscard]] KeyRef BPPTREE_MINMAX() const {
            if (this->length == 0) {
                throw std::out_of_range("cannot call " BPPTREE_MINMAXSTR " on empty node");
            }
            IndexType min_max_index = BPPTREE_CONCAT(BPPTREE_MINMAX, _excluding)();
            return extractor(this->values[min_max_index]);
        }

        [[nodiscard]] KeyRef BPPTREE_MINMAX(uint64_t begin, uint64_t end) const {
            IndexType beginIndex = this->getIndex(begin);
            IndexType endIndex = this->getIndex(end);
            if (beginIndex > endIndex) {
                throw std::out_of_range("cannot call " BPPTREE_MINMAXSTR " on empty range");
            }
            IndexType min_max_index = BPPTREE_CONCAT(BPPTREE_MINMAX, _excluding)(beginIndex, endIndex);
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
            if (this->length == 0) {
                throw std::out_of_range("cannot call " BPPTREE_MINMAXSTR " on empty node");
            }
            IndexType min_max_index = BPPTREE_CONCAT(BPPTREE_MINMAX, _excluding)();
            this->setIndex(it.iter, min_max_index);
            it.leaf = &this->self();
        }

        template <typename It>
        void BPPTREE_MINMAX(It& it, uint64_t begin, uint64_t end) const {
            IndexType beginIndex = this->getIndex(begin);
            IndexType endIndex = this->getIndex(end);
            if (beginIndex > endIndex) {
                throw std::out_of_range("cannot call " BPPTREE_MINMAXSTR " on empty range");
            }
            IndexType min_max_index = BPPTREE_CONCAT(BPPTREE_MINMAX, _excluding)(beginIndex, endIndex);
            this->setIndex(it.iter, min_max_index);
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

        InternalNode() noexcept = default;

        InternalNode(InternalNode const& other) noexcept : Parent(other), BPPTREE_MINMAXS(other.BPPTREE_MINMAXS, other.length) {}

        ~InternalNode() {
            if constexpr (!std::is_trivially_destructible_v<Key>) {
                for (IndexType i = 0; i < this->length; ++i) {
                    BPPTREE_MINMAXS.destruct(i);
                }
            }
        }

        void eraseElement2(IndexType index) {
            BPPTREE_MINMAXS.destruct(index);
            Parent::eraseElement2(index);
        }

        void moveElement2(IndexType destIndex, NodeType& source, IndexType sourceIndex) {
            BPPTREE_MINMAXS.set(destIndex, this->length, source.BPPTREE_MINMAXS.move(sourceIndex));
            Parent::moveElement2(destIndex, source, sourceIndex);
        }

        void copyElement2(IndexType destIndex, NodeType const& source, IndexType sourceIndex) {
            BPPTREE_MINMAXS.set(destIndex, this->length, source.BPPTREE_MINMAXS[sourceIndex]);
            Parent::copyElement2(destIndex, source, sourceIndex);
        }

        void replaceElement2(IndexType index, InfoType<ChildType>& t) {
            BPPTREE_MINMAXS[index] = std::move(t.BPPTREE_MINMAX);
            Parent::replaceElement2(index, t);
        }

        void setElement2(IndexType index, InfoType<ChildType>& t) {
            BPPTREE_MINMAXS.set(index, this->length, std::move(t.BPPTREE_MINMAX));
            Parent::setElement2(index, t);
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

        void computeDeltaSplit2(SplitType<ChildType> const& split, InfoType<NodeType>& nodeInfo, IndexType index) const {
            auto [BPPTREE_MINMAX, min_max_index] = BPPTREE_CONCAT(BPPTREE_MINMAX, _excluding)(index);
            if (BPPTREE_MINMAX == nullptr) {
                nodeInfo.BPPTREE_MINMAX = std::min(split.left.BPPTREE_MINMAX, split.right.BPPTREE_MINMAX, comp);
            } else {
                if (comp(split.left.BPPTREE_MINMAX, *BPPTREE_MINMAX)) {
                    BPPTREE_MINMAX = &split.left.BPPTREE_MINMAX;
                }
                if (comp(split.right.BPPTREE_MINMAX, *BPPTREE_MINMAX)) {
                    BPPTREE_MINMAX = &split.right.BPPTREE_MINMAX;
                }
                nodeInfo.BPPTREE_MINMAX = *BPPTREE_MINMAX;
            }
            Parent::computeDeltaSplit2(split, nodeInfo, index);
        }

        void computeDeltaReplace2(InfoType<ChildType> const& update, InfoType<NodeType>& nodeInfo, IndexType index) const {
            auto [BPPTREE_MINMAX, min_max_index] = BPPTREE_CONCAT(BPPTREE_MINMAX, _excluding)(index);
            if (BPPTREE_MINMAX != nullptr && comp(*BPPTREE_MINMAX, update.BPPTREE_MINMAX)) {
                nodeInfo.BPPTREE_MINMAX = *BPPTREE_MINMAX;
            } else {
                nodeInfo.BPPTREE_MINMAX = update.BPPTREE_MINMAX;
            }
            Parent::computeDeltaReplace2(update, nodeInfo, index);
        }

        void computeDeltaErase2(IndexType index, InfoType<NodeType>& nodeInfo) const {
            auto [BPPTREE_MINMAX, min_max_index] = BPPTREE_CONCAT(BPPTREE_MINMAX, _excluding)(index);
            if (BPPTREE_MINMAX != nullptr) {
                nodeInfo.BPPTREE_MINMAX = *BPPTREE_MINMAX;
            }
            Parent::computeDeltaErase2(index, nodeInfo);
        }

        [[nodiscard]] KeyRef BPPTREE_MINMAX() const {
            if (this->length == 0) {
                throw std::out_of_range("cannot call " BPPTREE_MINMAXSTR " on empty node");
            }
            auto [BPPTREE_MINMAX, min_max_index] = BPPTREE_CONCAT(BPPTREE_MINMAX, _excluding)();
            return *BPPTREE_MINMAX;
        }

        [[nodiscard]] Key const& BPPTREE_CONCAT(BPPTREE_MINMAX, _internal)(Key const& smallest, IndexType beginIndex, IndexType endIndex) const {
            auto [BPPTREE_MINMAX, min_max_index] = BPPTREE_CONCAT(BPPTREE_MINMAX, _excluding)(beginIndex, endIndex);
            return BPPTREE_MINMAX != nullptr && comp(*BPPTREE_MINMAX, smallest) ? *BPPTREE_MINMAX : smallest;
        }

        [[nodiscard]] KeyRef BPPTREE_CONCAT(BPPTREE_MINMAX, _after)(uint64_t begin) const {
            IndexType beginIndex = this->getIndex(begin);
            decltype(auto) first = this->pointers[beginIndex]->BPPTREE_CONCAT(BPPTREE_MINMAX, _after)(begin);
            return BPPTREE_CONCAT(BPPTREE_MINMAX, _internal)(first, beginIndex+1, this->length);
        }

        [[nodiscard]] KeyRef BPPTREE_CONCAT(BPPTREE_MINMAX, _before)(uint64_t end) const {
            IndexType endIndex = this->getIndex(end);
            decltype(auto) last = this->pointers[endIndex]->BPPTREE_CONCAT(BPPTREE_MINMAX, _before)(end);
            return BPPTREE_CONCAT(BPPTREE_MINMAX, _internal)(last, 0, endIndex);
        }

        [[nodiscard]] KeyRef BPPTREE_MINMAX(uint64_t begin, uint64_t end) const {
            IndexType beginIndex = this->getIndex(begin);
            IndexType endIndex = this->getIndex(end);
            if (beginIndex == endIndex) {
                return this->pointers[beginIndex]->BPPTREE_MINMAX(begin, end);
            }
            decltype(auto) first = this->pointers[beginIndex]->BPPTREE_CONCAT(BPPTREE_MINMAX, _after)(begin);
            decltype(auto) last = this->pointers[endIndex]->BPPTREE_CONCAT(BPPTREE_MINMAX, _before)(end);
            decltype(auto) tmp = std::min(first, last, comp);
            return BPPTREE_CONCAT(BPPTREE_MINMAX, _internal)(tmp, beginIndex+1, endIndex);
        }

        template <typename It>
        void BPPTREE_MINMAX(It& it) const {
            if (this->length == 0) {
                throw std::out_of_range("cannot call " BPPTREE_MINMAXSTR " on empty node");
            }
            auto [BPPTREE_MINMAX, min_max_index] = BPPTREE_CONCAT(BPPTREE_MINMAX, _excluding)();
            this->setIndex(it.iter, min_max_index);
            this->pointers[min_max_index]->BPPTREE_MINMAX(it);
        }

        template <typename It>
        void BPPTREE_CONCAT(BPPTREE_MINMAX, _internal)(It& it, It const& smallest, IndexType beginIndex, IndexType endIndex) const {
            auto [BPPTREE_MINMAX, min_max_index] = BPPTREE_CONCAT(BPPTREE_MINMAX, _excluding)(beginIndex, endIndex);
            if (BPPTREE_MINMAX != nullptr && comp(*BPPTREE_MINMAX, extractor(smallest.get()))) {
                this->setIndex(it.iter, min_max_index);
                this->pointers[min_max_index]->BPPTREE_MINMAX(it);
            } else {
                it = smallest;
            }
        }

        template <typename It>
        void BPPTREE_CONCAT(BPPTREE_MINMAX, _after)(It& it, uint64_t begin) const {
            IndexType beginIndex = this->getIndex(begin);
            It b(it);
            this->setIndex(b.iter, beginIndex);
            this->pointers[beginIndex]->BPPTREE_CONCAT(BPPTREE_MINMAX, _after)(b, begin);
            BPPTREE_CONCAT(BPPTREE_MINMAX, _internal)(it, b, beginIndex + 1, this->length);
        }

        template <typename It>
        void BPPTREE_CONCAT(BPPTREE_MINMAX, _before)(It& it, uint64_t end) const {
            IndexType endIndex = this->getIndex(end);
            It e(it);
            this->setIndex(e.iter, endIndex);
            this->pointers[endIndex]->BPPTREE_CONCAT(BPPTREE_MINMAX, _before)(e, end);
            BPPTREE_CONCAT(BPPTREE_MINMAX, _internal)(it, e, 0, endIndex);
        }

        template <typename It>
        void BPPTREE_MINMAX(It& it, uint64_t begin, uint64_t end) const {
            IndexType beginIndex = this->getIndex(begin);
            IndexType endIndex = this->getIndex(end);
            if (beginIndex == endIndex) {
                this->setIndex(it.iter, beginIndex);
                this->pointers[beginIndex]->BPPTREE_MINMAX(it, begin, end);
                return;
            }
            It b(it);
            this->setIndex(b.iter, beginIndex);
            this->pointers[beginIndex]->BPPTREE_CONCAT(BPPTREE_MINMAX, _after)(b, begin);
            It e(it);
            this->setIndex(e.iter, endIndex);
            this->pointers[endIndex]->BPPTREE_CONCAT(BPPTREE_MINMAX, _before)(e, end);
            It const& tmp = comp(extractor(b.get()), extractor(e.get())) ? b : e;
            BPPTREE_CONCAT(BPPTREE_MINMAX, _internal)(it, tmp, beginIndex + 1, endIndex);
        }
    };

    template <typename Parent>
    struct NodeInfo : public Parent {
        Key BPPTREE_MINMAX{};

        NodeInfo() noexcept = default;

        template <typename P>
        NodeInfo(P&& p, const bool changed) noexcept : Parent(std::forward<P>(p), changed), BPPTREE_MINMAX(changed ? this->ptr->BPPTREE_MINMAX() : p->BPPTREE_MINMAX()) {}
    };

    template <typename Parent>
    struct Transient : public Parent {
        template <typename... Us>
        explicit Transient(Us&&... us) noexcept : Parent(std::forward<Us>(us)...) {}
    };

    template <typename Parent>
    struct Persistent : public Parent {
        template <typename... Us>
        explicit Persistent(Us&&... us) noexcept : Parent(std::forward<Us>(us)...) {}
    };
};
}
