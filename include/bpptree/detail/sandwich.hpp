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
struct top {
    using SelfType = Derived;
    SelfType& self() & { return static_cast<SelfType&>(*this); }
    SelfType&& self() && { return static_cast<SelfType&&>(*this); }
    SelfType const& self() const& { return static_cast<SelfType const&>(*this); }
    SelfType const&& self() const&& { return static_cast<SelfType const&&>(*this); }
};

template <template <typename...> typename Mixin, typename... Args>
struct curry {
    template <typename Base>
    using mixin = Mixin<Base, Args...>;
};

template <template <typename, auto...> typename Mixin, auto... Args>
struct curry2 {
    template <typename Base>
    using mixin = Mixin<Base, Args...>;
};

template <typename Parent, template<typename> typename Head, template<typename> typename... Tail>
struct chain {
    using result = typename chain<Parent, Tail...>::type;
    using type = Head<result>;
};

template <typename Parent, template<typename> typename Head>
struct chain<Parent, Head> {
    using type = Head<Parent>;
};

template <template <typename> typename... Mixins>
struct Mixin {

    template <typename T>
    using chain_t = typename chain<top<T>, Mixins...>::type;

    struct type : public chain_t<type> {

        template <typename... Rest>
        explicit constexpr type(Rest&&... rest) noexcept : chain_t<type>(std::forward<Rest>(rest)...) {}

        template <typename Other>
        type& operator=(Other&& other) noexcept {
            chain_t<type>::operator=(std::forward<Other>(other));
            return *this;
        }

        type() noexcept = default;
        type(type const&) noexcept = default;
        type(type&&) noexcept = default;
        type& operator=(type const&) noexcept = default;
        type& operator=(type&&) noexcept = default;
        ~type() noexcept = default;
    };
};

template <template <typename> typename... Mixins>
using MixinT = typename Mixin<Mixins...>::type;

template <typename Parent, template <typename> typename... Mixins>
using chain_t = typename Mixin<Mixins...>::template chain_t<Parent>;
} //end namespace bpptree::detail
