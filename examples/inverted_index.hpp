#pragma once

#include <cstdint>
#include <utility>
#include "bpptree/bpptree.hpp"
#include "bpptree/ordered.hpp"
#include "bpptree/summed.hpp"
#include "bpptree/indexed.hpp"

template <typename Term>
struct InvertedIndex {

    using Value = std::pair<Term, uint32_t>;
    
    using TermList = typename bpptree::BppTree<Value>::template mixins<
        bpptree::OrderedBuilder<>,
        bpptree::SummedBuilder<bpptree::detail::PairExtractor<1>>>;

    using DocList = typename bpptree::BppTree<uint32_t>::template mixins<bpptree::IndexedBuilder<uint32_t>>;

    template <typename Derived>
    struct Shared {

        [[nodiscard]] Derived& self() {
            return *static_cast<Derived*>(this);
        }

        [[nodiscard]] Derived const& self() const {
            return *static_cast<Derived const*>(this);
        }

        template <typename It, std::enable_if_t<bpptree::detail::IsTreeIterator<It>::value, bool> = true>
        [[nodiscard]] auto term_doc_iterator(It const& it) const {
            auto measure = self().term_list().sum_inclusive(it);
            return std::make_pair(
                    self().doc_list().find_index(measure - it->second),
                    self().doc_list().find_index(measure)
            );
        }

        [[nodiscard]] auto term_doc_iterator(Term const& term) const {
            auto it = self().term_list().find(term);
            return term_doc_iterator2(it);
        }
    };

    class Persistent;

    class Transient : public Shared<Transient> {
        friend struct Shared<Transient>;

        typename TermList::Transient term_list_{};
        typename DocList::Transient doc_list_{};
    public:
        Transient() = default;
    private:
        Transient(typename TermList::Transient&& term_list, typename DocList::Transient&& doc_list) :
                Shared<Transient>(), term_list_(std::move(term_list)), doc_list_(std::move(doc_list)) {}

        [[nodiscard]] typename DocList::Transient const& doc_list() const {
            return doc_list_;
        }

    public:
        [[nodiscard]] Persistent persistent() & {
            return Persistent(term_list_.persistent(), doc_list_.persistent());
        }

        [[nodiscard]] Persistent persistent() && {
            return Persistent(std::move(term_list_).persistent(), std::move(doc_list_).persistent());
        }

        [[nodiscard]] typename TermList::Transient const& term_list() const {
            return term_list_;
        }

        template <typename T>
        void insert(T&& term, uint32_t doc_id) {
            auto it = term_list_.lower_bound(term);
            if (it->first != term) {
                term_list_.insert(it, std::forward<T>(term), 0u);
            }
            auto [begin, end] = this->term_doc_iterator(it);
            auto doc_it = std::lower_bound(begin, end, doc_id);
            if (doc_it == end || *doc_it != doc_id) {
                doc_list_.insert(doc_it, doc_id);
                term_list_.update(it, [](auto const& p){ return std::make_pair(p.first, p.second + 1); });
            }
        }
    };

    class Persistent : public Shared<Persistent> {
        friend struct Shared<Persistent>;
        
        typename TermList::Persistent term_list_{};
        typename DocList::Persistent doc_list_{};
    public:
        Persistent() = default;
    private:
        Persistent(typename TermList::Persistent&& term_list, typename DocList::Persistent&& doc_list) :
            Shared<Persistent>(), term_list_(std::move(term_list)), doc_list_(std::move(doc_list)) {}

        [[nodiscard]] typename DocList::Persistent const& doc_list() const {
            return doc_list_;
        }

    public:
        [[nodiscard]] Transient transient() const& {
            return Transient(term_list_.transient(), doc_list_.transient());
        }

        [[nodiscard]] Transient transient() && {
            return Transient(std::move(term_list_).transient(), std::move(doc_list_).transient());
        }

        [[nodiscard]] typename TermList::Persistent const& term_list() const {
            return term_list_;
        }

        template <typename T>
        [[nodiscard]] Persistent insert(T&& term, uint32_t doc_id) const {
            Transient transient(transient());
            transient.insert(std::forward<T>(term), doc_id);
            return std::move(transient).persistent();
        }
    };
};
