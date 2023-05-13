//
// B++ Tree: A B+ Tree library written in C++
// Copyright (C) 2023 Jeff Plaisance
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#pragma once

#include <type_traits>
#include "nodeptr.hpp"
#include "helpers.hpp"

namespace bpptree::detail {

template <typename LeafNode, template<int> typename InternalNode, int max_depth>
struct ModifyTypes {
    template <typename TreeType>
    static void collapse(TreeType& tree) {
        while(tree.dispatch([](auto& tree, auto& root){
            if constexpr (std::decay_t<decltype(root)>::Type::depth > 1) {
                if (root->length == 1) {
                    // have to call copy constructor here on pointer or else assignment to variant destroys root
                    // (and therefore destroys root->pointers[0]) before copy can happen
                    tree.root_variant = NodePtr(root->pointers[0]);
                    return true;
                }
            }
            return false;
        }));
    }

    template <typename TreeType, typename NodeType>
    struct DoReplace {
        TreeType& tree;
        NodePtr<NodeType>& root;

        DoReplace(TreeType& tree, NodePtr<NodeType>& root) : tree(tree), root(root) {}

        template <typename ReplaceType>
        void operator()(ReplaceType&& replace) {
            bool do_collapse = false;
            if (replace.delta.ptr_changed) {
                if constexpr (NodeType::depth > 1) {
                    if (replace.delta.ptr->length == 1) {
                        do_collapse = true;
                    }
                }
                tree.root_variant = std::move(replace.delta.ptr);
            } else if constexpr (NodeType::depth > 1) {
                if (root->length == 1) {
                    do_collapse = true;
                }
            }
            if (do_collapse) {
                collapse(tree);
            }
        }
    };

    template <typename TreeType, typename NodeType>
    struct DoSplit {
        TreeType& tree;
        NodePtr<NodeType>& root;
        uint64_t& iter;

        DoSplit(TreeType& tree, NodePtr<NodeType>& root, uint64_t& iter) : tree(tree), root(root), iter(iter) {}

        template <typename SplitType>
        void operator()([[maybe_unused]] SplitType&& split) {
            if constexpr (NodeType::depth < max_depth) {
                using NewRootType = InternalNode<NodeType::depth + 1>;
                auto root_node = make_ptr<NewRootType>();
                if (!split.left.ptr_changed) {
                    split.left.ptr = std::move(root);
                    split.left.ptr_changed = true;
                }
                root_node->set_element(0, split.left);
                root_node->set_element(1, split.right);
                root_node->length = 2;
                root_node->set_index(iter, split.new_element_left ? 0 : 1);
                tree.root_variant = std::move(root_node);
            } else {
#ifdef BPPTREE_SAFETY_CHECKS
                throw std::logic_error("maximum depth exceeded");
#endif
            }
        }
    };

    template <typename TreeType>
    struct DoErase {
        TreeType& tree;

        explicit DoErase(TreeType& tree) : tree(tree) {}

        void operator()() {
            tree.root_variant = make_ptr<LeafNode>();
        }
    };

    template <typename Operation>
    struct Modify {
        template <typename TreeType, typename NodeType, typename F, typename T, typename... Us>
        uint64_t operator()(TreeType& tree, NodePtr<NodeType>& root, F&& finder, T const& search_val, Us&&... params) {
            uint64_t ret = 0;
            Operation()(*root, search_val, finder,
                DoReplace<TreeType, NodeType>(tree, root),
                DoSplit<TreeType, NodeType>(tree, root, ret),
                DoErase<TreeType>(tree),
                tree.tree_size,
                ret,
                true,
                std::forward<Us>(params)...
            );
            ++tree.mod_count;
            return ret;
        }
    };
};
}
