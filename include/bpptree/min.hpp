#pragma once

#include <array>
#include <type_traits>

#include "bpptree/detail/helpers.hpp"
#include "bpptree/detail/uninitialized_array.hpp"

#define BPPTREE_MINMAX min
#define BPPTREE_MINMAX_UPPER Min
#define BPPTREE_MINMAXSTR "min"
#include "bpptree/detail/minmax.hpp"

namespace bpptree {
namespace detail {

/**
 * Min mixin adds support for finding the minimum value in a B++ tree in O(log N) time
 * You can find just the min value using the min() method or get an iterator pointing to the min element with min_element()
 * You can also find the min value/element in a subrange of the tree with min(it1, it2) or min_element(it1, it2)
 * @tparam Value the value type of the B++ tree
 * @tparam KeyExtractor a class which implements operator()(Value) to return the field within the value type that is used for determining the min
 * @tparam Comp a class which implements operator()(A const& a, B const& b) to return true if a < b and false otherwise
 */
template <typename Value, typename KeyExtractor = ValueExtractor, typename Comp = MinComparator>
struct Min {
private:
    static constexpr KeyExtractor extractor{};

    using KeyRef = decltype(extractor(std::declval<Value>()));

    using Detail = MinDetail<Value, KeyExtractor, Comp>;
public:
    template <typename Parent>
    using LeafNode = typename Detail::template LeafNode<Parent>;

    template <typename Parent, auto internal_size>
    using InternalNode = typename Detail::template InternalNode<Parent, internal_size>;

    template <typename Parent>
    using NodeInfo = typename Detail::template NodeInfo<Parent>;

    template <typename Parent>
    using Transient = typename Detail::template Transient<Parent>;

    template <typename Parent>
    using Persistent = typename Detail::template Persistent<Parent>;

    template <typename Parent>
    struct Shared : public Parent {
        template <typename... Us>
        explicit Shared(Us&&... us) noexcept : Parent(std::forward<Us>(us)...) {}

        using iterator = typename Parent::iterator;
        using const_iterator = typename Parent::const_iterator;

        [[nodiscard]] KeyRef min() const;

        template <typename It>
        [[nodiscard]] KeyRef min(It const& begin, It const& end) const;

    private:
        template <typename It>
        void seek_min(It& it) const;

        template <typename OutIt, typename InIt>
        void seek_min(OutIt& out, InIt const& begin, InIt const& end) const;

    public:
        [[nodiscard]] iterator min_element();

        template <typename It>
        [[nodiscard]] iterator min_element(It const& begin, It const& end);

        [[nodiscard]] const_iterator min_element_const() const;

        template <typename It>
        [[nodiscard]] const_iterator min_element_const(It const& begin, It const& end) const;

        [[nodiscard]] const_iterator min_element() const;

        template <typename It>
        [[nodiscard]] const_iterator min_element(It const& begin, It const& end) const;
    };
};
#include "bpptree/detail/minmax2.ipp"

template <typename Extractor = ValueExtractor, typename Comp = MinComparator>
struct MinBuilder {
    template <typename T>
    using compare = MinBuilder<Extractor, T>;

    template <typename T>
    using extractor = MinBuilder<T, Comp>;

    template <typename Value>
    using build = Min<Value, Extractor, Comp>;
};
} //end namespace detail
using detail::Min;
using detail::MinBuilder;
} //end namespace bpptree

#undef BPPTREE_MINMAX
#undef BPPTREE_MINMAX_UPPER
#undef BPPTREE_MINMAXSTR
