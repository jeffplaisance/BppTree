#pragma once
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunknown-pragmas"

#include <vector>
#include <iostream>
#include "bpptree/bpptree.hpp"
#include "bpptree/summed.hpp"
#include "bpptree/indexed.hpp"
#include "bpptree/ordered.hpp"

using namespace bpptree::detail;

inline constexpr int num_ints_small = 100*1000;
inline constexpr int num_ints_large = 1000*1000;

template <typename T>
struct Vector : std::vector<T> {

#pragma clang diagnostic push
#pragma ide diagnostic ignored "HidingNonVirtualFunction"
    template <typename I, std::enable_if_t<std::is_integral_v<I>, bool> = true>
    decltype(auto) operator[](I const index) noexcept {
        return std::vector<T>::operator[](static_cast<size_t const>(index));
    }
#pragma clang diagnostic pop

#pragma clang diagnostic push
#pragma ide diagnostic ignored "HidingNonVirtualFunction"
    template <typename I, std::enable_if_t<std::is_integral_v<I>, bool> = true>
    decltype(auto) operator[](I const index) const noexcept {
        return std::vector<T>::operator[](static_cast<size_t const>(index));
    }
#pragma clang diagnostic pop
};


template <typename T, size_t n>
struct RandInts {

    static Vector<T> get_ints() {
        Vector<T> rand_ints{};
        for (size_t i = 0; i < n; ++i) {
            rand_ints.push_back(static_cast<T>(i));
        }
        for (auto it = rand_ints.begin(); it != rand_ints.end(); ++it) {
            std::swap(*it, *(it + rand() % (rand_ints.end() - it)));
        }
        return rand_ints;
    }

    static inline Vector<T> const ints = get_ints();
};

template <
        typename Value,
        typename SumType = Value,
        typename Extractor = CastingExtractor<SumType>>
using SummedIndexedTree = typename BppTree<Value, 512, 128, 8>
        ::template mixins<SummedBuilder<Extractor>, IndexedBuilder<>>;

template <
        typename Value,
        typename KeyValueExtractor,
        typename SumType = Value,
        typename Extractor = ValueExtractor,
        typename Comp = MinComparator,
        bool binary_search = false>
using OrderedTree = typename BppTree<Value, 512, 512, 6>
        ::template mixins<
                typename OrderedBuilder<>
                        ::extractor<KeyValueExtractor>
                        ::template compare<Comp>
                        ::template binary_search<binary_search>,
                SummedBuilder<WrappedCastingExtractor<Extractor, SumType>>>;

//do these functions assume 2's complement? yes. do i care that c++17 technically allows for other integer representations? no.
template <typename T, std::enable_if_t<std::is_integral_v<T>, bool> = true>
inline auto signed_cast(T t) {
    return static_cast<std::make_signed_t<T>>(t);
}

template <typename T, std::enable_if_t<std::is_integral_v<T>, bool> = true>
inline auto unsigned_cast(T t) {
    return static_cast<std::make_unsigned_t<T>>(t);
}

#pragma clang diagnostic pop
