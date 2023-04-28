//
// B++ Tree: A B+ Tree library written in C++
// Copyright (C) 2023 Jeff Plaisance
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#pragma once

#include <type_traits>
#include <cstdint>
#include <utility>
#include <vector>
#include <limits>
#include "common.hpp"
#include "helpers.hpp"
#include "proxy_operators.hpp"

namespace bpptree::detail {

template <typename Tree, bool is_const, typename = void>
struct IteratorBase {
    explicit IteratorBase(std::conditional_t<is_const, Tree const, Tree>&) noexcept {}
};

template <typename Tree, bool is_const>
struct IteratorBase<Tree, is_const, std::enable_if_t<IsTransientTree<Tree>::value>> {
    mutable uint64_t mod_count;

    explicit IteratorBase(std::conditional_t<is_const, Tree const, Tree>& tree) noexcept : mod_count(tree.mod_count) {}
};

template <typename Value, typename Tree, typename LeafNode, bool is_const, bool reverse>
struct IteratorDetail : public IteratorBase<Tree, is_const> {
    using Parent = IteratorBase<Tree, is_const>;

    using TreeType = std::conditional_t<is_const, Tree const, Tree>;

    static constexpr bool is_transient_tree = IsTransientTree<TreeType>::value;

    static constexpr int direction = reverse ? -1 : 1;

    static constexpr bool is_reversed = reverse;

    static constexpr uint64_t rend = std::numeric_limits<uint64_t>::max();

    uint64_t iter = 0;
    TreeType* tree;
    mutable LeafNode const* leaf;

    explicit IteratorDetail(TreeType& tree) noexcept : Parent(tree), tree(&tree) {}

    [[nodiscard]] Value const& get() const {
        if constexpr (is_transient_tree) {
            if (this->mod_count != tree->mod_count) {
                std::as_const(*tree).dispatch([this](auto const& root) {
                    // make copy of iter since this is const and iter is not mutable.
                    // advancing by 0 won't actually change tmp_iter.
                    uint64_t tmp_iter = iter;
                    root->advance(leaf, tmp_iter, 0);
                });
                this->mod_count = tree->mod_count;
            }
        }
        return leaf->get_iter(iter);
    }

    struct ProxyRef : public ProxyOperators<ProxyRef> {
    private:
        friend struct ProxyOperators<ProxyRef>;

        IteratorDetail const& it;

        ProxyRef(const ProxyRef& other) noexcept = default;
        ProxyRef(ProxyRef&& other) noexcept = default;
        ProxyRef& operator=(const ProxyRef& other) noexcept = default; //NOLINT
        ProxyRef& operator=(ProxyRef&& other) noexcept = default; //NOLINT

        template <typename F>
        ProxyRef& invoke_compound_assignment(F&& f) {
            it.tree->update(it, f);
            return *this;
        }

        template <typename F>
        ProxyRef& invoke_pre(F&& f) {
            it.tree->update(it, [&f](auto const& v){
                Value copy(v);
                f(copy);
                return copy;
            });
            return *this;
        }

        template <typename F>
        Value invoke_post(F&& f) {
            Value ret;
            it.tree->update(it, [&f, &ret](auto const& v){
                Value copy(v);
                ret = f(copy);
                return copy;
            });
            return ret;
        }
    public:
        explicit ProxyRef(IteratorDetail const& it) noexcept : it(it) {}
        ~ProxyRef() noexcept = default;

        template <typename T>
        ProxyRef& operator=(T&& t) {
            Value const& v = t;
            it.tree->assign(it, v);
            return *this;
        }

        operator Value const&() const { //NOLINT
            return it.get();
        }

        [[nodiscard]] Value const& get() const {
            return it.get();
        }

        friend void swap(ProxyRef&& a, ProxyRef&& b) {
            Value tmp = a;
            Value const& bv = b;
            a.it.tree->assign(a.it, bv);
            b.it.tree->assign(b.it, std::move(tmp));
        }
    };

    template <bool c = is_const, std::enable_if_t<!c, bool> = true>
    [[nodiscard]] ProxyRef operator*() {
        static_assert(c == is_const);
        return ProxyRef(*this);
    }

    template <bool c = is_const, std::enable_if_t<c, bool> = true>
    [[nodiscard]] Value const& operator*() const {
        static_assert(c == is_const);
        return get();
    }

    [[nodiscard]] Value const* operator->() const {
        return &get();
    }

    void advance(ssize n) {
        n *= direction;
        ssize remainder;
        if (iter == rend) {
            if (n <= 0) {
                return;
            }
            iter = 0;
            if (n == 1) {
                std::as_const(*tree).dispatch([this](auto const& root) { root->seek_begin(leaf, iter); });
                if constexpr (is_transient_tree) {
                    this->mod_count = tree->mod_count;
                }
                return;
            }
            remainder = n - 1;
        } else if constexpr (!is_transient_tree) { //NOLINT
            remainder = leaf->advance(leaf, iter, n);
        } else if (this->mod_count == tree->mod_count) {
            remainder = leaf->advance(leaf, iter, n);
        } else {
            remainder = n;
        }
        if (remainder != 0) {
            std::as_const(*tree).dispatch([this, remainder](auto const& root) {
                auto r = root->advance(leaf, iter, remainder);
                if (r > 0) {
                    root->seek_end(leaf, iter);
                }
                if (r < 0) {
                    leaf = nullptr;
                    iter = rend;
                }
            });
            if constexpr (is_transient_tree) {
                this->mod_count = tree->mod_count;
            }
        }
    }

    auto& operator++() {
        advance(1);
        return *this;
    }

    auto operator++(int) & { //NOLINT
        auto ret = *this;
        advance(1);
        return ret;
    }

    auto& operator--() {
        advance(-1);
        return *this;
    }

    auto operator--(int) & { //NOLINT
        auto ret = *this;
        advance(-1);
        return ret;
    }

    auto& operator+=(ssize n) {
        advance(n);
        return *this;
    }

    auto& operator-=(ssize n) {
        advance(-n);
        return *this;
    }

    [[nodiscard]] auto operator+(ssize n) const {
        IteratorDetail ret(*this);
        ret += n;
        return ret;
    }

    [[nodiscard]] auto operator-(ssize n) const {
        IteratorDetail ret(*this);
        ret -= n;
        return ret;
    }

    [[nodiscard]] Value const& operator[](int64_t n) const {
        return *(*this + n);
    }

    template <typename T, std::enable_if_t<IsTreeIterator<T>::value && IsIndexedTree<TreeType, T const&>::value, bool> = true>
    [[nodiscard]] auto operator-(T const& it) const {
        if constexpr (T::is_reversed) {
            ssize ret = iter == rend ? 1 : -static_cast<ssize>(tree->order(*this));
            ret += it.iter == rend ? -1 : static_cast<ssize>(tree->order(it));
            return ret;
        } else {
            return static_cast<ssize>(tree->order(*this)) - static_cast<ssize>(tree->order(it));
        }
    }

    template <typename RHS>
    [[nodiscard]] bool operator<(RHS const& rhs) const {
        static_assert(reverse == RHS::is_reversed);
        if (&tree->self() != &rhs.tree->self()) {
            throw std::logic_error("cannot compare iterators from different trees");
        }
        return reverse ^ (iter + 1 < rhs.iter + 1);
    }

    template <typename RHS>
    [[nodiscard]] bool operator==(RHS const& rhs) const {
        if (&tree->self() != &rhs.tree->self()) {
            return false;
        }
        return iter == rhs.iter;
    }

    template <typename RHS>
    [[nodiscard]] bool operator>(RHS const& rhs) const {
        return rhs < *this;
    }

    template <typename RHS>
    [[nodiscard]] bool operator<=(RHS const& rhs) const {
        return !(rhs < *this); //NOLINT
    }

    template <typename RHS>
    [[nodiscard]] bool operator>=(RHS const& rhs) const {
        return !(*this < rhs); //NOLINT
    }

    template <typename RHS>
    [[nodiscard]] bool operator!=(RHS const& rhs) const {
        return !(*this == rhs); //NOLINT
    }

    [[nodiscard]] std::vector<uint16_t> get_indexes() const {
        std::vector<uint16_t> ret;
        std::as_const(*tree).dispatch([this, &ret](auto const& root) { root->get_indexes(iter, ret); });
        return ret;
    }

    using difference_type = ssize;

    using value_type = Value;

    using pointer = void;

    using reference = std::conditional_t<is_const, Value const&, ProxyRef>;

    using iterator_category = std::conditional_t<
            IsIndexedTree<TreeType, IteratorDetail>::value,
            std::random_access_iterator_tag,
            std::bidirectional_iterator_tag>;
};
} //end namespace bpptree::detail
