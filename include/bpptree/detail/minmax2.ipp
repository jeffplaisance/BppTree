//
// B++ Tree: A B+ Tree library written in C++
// Copyright (C) 2023 Jeff Plaisance
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

template <typename Value, typename KeyExtractor, typename Comp>
template <typename Parent>
typename BPPTREE_MINMAX_UPPER<Value, KeyExtractor, Comp>::KeyRef BPPTREE_MINMAX_UPPER<Value, KeyExtractor, Comp>::Shared<Parent>::BPPTREE_MINMAX() const {
    return this->self().dispatch(
            [](auto const& root) -> decltype(auto) { return root->BPPTREE_MINMAX(); }
    );
}

template <typename Value, typename KeyExtractor, typename Comp>
template <typename Parent>
template <typename It>
typename BPPTREE_MINMAX_UPPER<Value, KeyExtractor, Comp>::KeyRef BPPTREE_MINMAX_UPPER<Value, KeyExtractor, Comp>::Shared<Parent>::BPPTREE_MINMAX(const It& begin, const It& end) const {
    return this->self().dispatch(
            [&begin, &end](auto const& root) -> decltype(auto) {
                if constexpr (It::is_reversed) {
                    return root->BPPTREE_MINMAX((end-1).iter, (begin).iter);
                } else {
                    return root->BPPTREE_MINMAX(begin.iter, (end-1).iter);
                }
            }
    );
}

template <typename Value, typename KeyExtractor, typename Comp>
template <typename Parent>
template <typename It>
void BPPTREE_MINMAX_UPPER<Value, KeyExtractor, Comp>::Shared<Parent>::BPPTREE_CONCAT(seek_, BPPTREE_MINMAX)(It& it) const {
    this->self().dispatch(
            [&it](auto const& root) { root->BPPTREE_MINMAX(it); }
    );
}

template <typename Value, typename KeyExtractor, typename Comp>
template <typename Parent>
template <typename OutIt, typename InIt>
void BPPTREE_MINMAX_UPPER<Value, KeyExtractor, Comp>::Shared<Parent>::BPPTREE_CONCAT(seek_, BPPTREE_MINMAX)(OutIt& out, const InIt& begin, const InIt& end) const {
    this->self().dispatch(
            [&out, &begin, &end](auto const& root) {
                if constexpr (InIt::is_reversed) {
                    root->BPPTREE_MINMAX(out, (end-1).iter, begin.iter);
                } else {
                    root->BPPTREE_MINMAX(out, begin.iter, (end-1).iter);
                }
            }
    );
}

template <typename Value, typename KeyExtractor, typename Comp>
template <typename Parent>
typename BPPTREE_MINMAX_UPPER<Value, KeyExtractor, Comp>::template Shared<Parent>::iterator
        BPPTREE_MINMAX_UPPER<Value, KeyExtractor, Comp>::Shared<Parent>::BPPTREE_CONCAT(BPPTREE_MINMAX, _element)() {
    iterator ret(this->self());
    BPPTREE_CONCAT(seek_, BPPTREE_MINMAX)(ret);
    return ret;
}

template <typename Value, typename KeyExtractor, typename Comp>
template <typename Parent>
template <typename It>
typename BPPTREE_MINMAX_UPPER<Value, KeyExtractor, Comp>::template Shared<Parent>::iterator
        BPPTREE_MINMAX_UPPER<Value, KeyExtractor, Comp>::Shared<Parent>::BPPTREE_CONCAT(BPPTREE_MINMAX, _element)(const It& begin, const It& end) {
    iterator ret(this->self());
    BPPTREE_CONCAT(seek_, BPPTREE_MINMAX)(ret, begin, end);
    return ret;
}

template <typename Value, typename KeyExtractor, typename Comp>
template <typename Parent>
typename BPPTREE_MINMAX_UPPER<Value, KeyExtractor, Comp>::template Shared<Parent>::const_iterator
        BPPTREE_MINMAX_UPPER<Value, KeyExtractor, Comp>::Shared<Parent>::BPPTREE_CONCAT(BPPTREE_MINMAX, _element_const)() const {
    const_iterator ret(this->self());
    BPPTREE_CONCAT(seek_, BPPTREE_MINMAX)(ret);
    return ret;
}

template <typename Value, typename KeyExtractor, typename Comp>
template <typename Parent>
template <typename It>
typename BPPTREE_MINMAX_UPPER<Value, KeyExtractor, Comp>::template Shared<Parent>::const_iterator
        BPPTREE_MINMAX_UPPER<Value, KeyExtractor, Comp>::Shared<Parent>::BPPTREE_CONCAT(BPPTREE_MINMAX, _element_const)(const It& begin, const It& end) const {
    const_iterator ret(this->self());
    BPPTREE_CONCAT(seek_, BPPTREE_MINMAX)(ret, begin, end);
    return ret;
}

template <typename Value, typename KeyExtractor, typename Comp>
template <typename Parent>
typename BPPTREE_MINMAX_UPPER<Value, KeyExtractor, Comp>::template Shared<Parent>::const_iterator
        BPPTREE_MINMAX_UPPER<Value, KeyExtractor, Comp>::Shared<Parent>::BPPTREE_CONCAT(BPPTREE_MINMAX, _element)() const {
    return BPPTREE_CONCAT(BPPTREE_MINMAX, _element_const)();
}

template <typename Value, typename KeyExtractor, typename Comp>
template <typename Parent>
template <typename It>
typename BPPTREE_MINMAX_UPPER<Value, KeyExtractor, Comp>::template Shared<Parent>::const_iterator
        BPPTREE_MINMAX_UPPER<Value, KeyExtractor, Comp>::Shared<Parent>::BPPTREE_CONCAT(BPPTREE_MINMAX, _element)(const It& begin, const It& end) const {
    return BPPTREE_CONCAT(BPPTREE_MINMAX, _element_const)(begin, end);
}

#undef BPPTREE_MINMAXS
#undef BPPTREE_CONCAT
#undef CONCAT_INNER
