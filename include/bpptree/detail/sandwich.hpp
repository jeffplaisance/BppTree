//
// B++ Tree: A B+ Tree library written in C++
// Copyright (C) 2023 Jeff Plaisance
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#pragma once

namespace bpptree::detail {

template <typename Derived>
struct Top {
    using SelfType = Derived;
    SelfType& self() & { return static_cast<SelfType&>(*this); }
    SelfType&& self() && { return static_cast<SelfType&&>(*this); }
    SelfType const& self() const& { return static_cast<SelfType const&>(*this); }
    SelfType const&& self() const&& { return static_cast<SelfType const&&>(*this); }
};

template <template <typename, auto...> typename M, auto... args>
struct Curry {
    template <typename Base>
    using Mixin = M<Base, args...>;
};

template <typename Parent, template<typename> typename Head, template<typename> typename... Tail>
struct Chain3 {
    using Result = typename Chain3<Parent, Tail...>::Type;
    using Type = Head<Result>;
};

template <typename Parent, template<typename> typename Head>
struct Chain3<Parent, Head> {
    using Type = Head<Parent>;
};

template <typename Parent, template <typename> typename... Mixins>
struct Chain2 {
    using Type = typename Chain3<Parent, Mixins...>::Type;
};

template <typename Parent>
struct Chain2<Parent> {
    using Type = Parent;
};

template <typename Derived, template <typename> typename... Mixins>
using Chain = typename Chain2<Top<Derived>, Mixins...>::Type;
} //end namespace bpptree::detail
