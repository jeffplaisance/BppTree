#pragma once

#include <array>
#include <type_traits>

#include "bpptree/detail/helpers.hpp"
#include "bpptree/detail/uninitialized_array.hpp"

#define BPPTREE_MINMAX max
#define BPPTREE_MINMAX_UPPER Max
#define BPPTREE_MINMAXSTR "max"
#include "bpptree/detail/minmax.hpp"

namespace bpptree {
namespace detail {

/**
 * Max mixin adds support for finding the maximum value in a B++ tree in O(log N) time
 * You can find just the max value using the max() method or get an iterator pointing to the max element with max_element()
 * You can also find the max value/element in a subrange of the tree with max(it1, it2) or max_element(it1, it2)
 * @tparam Value the value type of the B++ tree
 * @tparam KeyExtractor a class which implements operator()(Value) to return the field within the value type that is used for determining the max
 * @tparam Comp a class which implements operator()(A const& a, B const& b) to return true if a > b and false otherwise
 */
template <typename Value, typename KeyExtractor = ValueExtractor, typename Comp = MaxComparator>
struct Max {
private:
    static constexpr KeyExtractor extractor{};

    using KeyRef = decltype(extractor(std::declval<Value>()));

    using Detail = MaxDetail<Value, KeyExtractor, Comp>;
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

        [[nodiscard]] KeyRef max() const;

        template <typename It>
        [[nodiscard]] KeyRef max(It const& begin, It const& end) const;

    private:
        template <typename It>
        void seek_max(It& it) const;

        template <typename OutIt, typename InIt>
        void seek_max(OutIt& out, InIt const& begin, InIt const& end) const;

    public:
        [[nodiscard]] iterator max_element();

        template <typename It>
        [[nodiscard]] iterator max_element(It const& begin, It const& end);

        [[nodiscard]] const_iterator max_element_const() const;

        template <typename It>
        [[nodiscard]] const_iterator max_element_const(It const& begin, It const& end) const;

        [[nodiscard]] const_iterator max_element() const;

        template <typename It>
        [[nodiscard]] const_iterator max_element(It const& begin, It const& end) const;
    };
};
#include "bpptree/detail/minmax2.ipp"

template <typename Extractor = ValueExtractor, typename Comp = MaxComparator>
struct MaxBuilder {
    template <typename T>
    using compare = MaxBuilder<Extractor, T>;

    template <typename T>
    using extractor = MaxBuilder<T, Comp>;

    template <typename Value>
    using build = Max<Value, Extractor, Comp>;
};
} //end namespace detail
using detail::Max;
using detail::MaxBuilder;
} //end namespace bpptree

#undef BPPTREE_MINMAX
#undef BPPTREE_MINMAX_UPPER
#undef BPPTREE_MINMAXSTR
