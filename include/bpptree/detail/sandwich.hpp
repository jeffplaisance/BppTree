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

    template <typename... Rest>
    explicit constexpr Top(Rest&&...) noexcept {}

    Top() noexcept = default;
};

template <template <typename, auto...> typename M, auto... Args>
struct Curry {
    template <typename Base>
    using Mixin = M<Base, Args...>;
};

template <typename>
using Nothing = void;

template <typename Parent, template<typename> typename Head = Nothing, template<typename> typename... Tail>
struct Chain2 {
    using Result = typename Chain2<Parent, Tail...>::Type;
    using Type = Head<Result>;
};

template <typename Parent, template<typename> typename Head>
struct Chain2<Parent, Head> {
    using Type = Head<Parent>;
};

template <typename Parent>
struct Chain2<Parent, Nothing> {
    using Type = Parent;
};

template <template <typename> typename... Mixins>
struct Mix {
    template <typename Derived>
    using Chain = typename Chain2<Top<Derived>, Mixins...>::Type;
};

template <typename Derived, template <typename> typename... Mixins>
using Chain = typename Mix<Mixins...>::template Chain<Derived>;
} //end namespace bpptree::detail
