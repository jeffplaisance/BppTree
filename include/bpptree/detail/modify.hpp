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

template <typename LeafNode, template<int> typename InternalNode, int maxDepth>
struct ModifyTypes {
    template <typename TreeType>
    static void collapse(TreeType& tree) {
        while(tree.dispatch([](auto& tree, auto& root){
            if constexpr (std::decay_t<decltype(root)>::Type::depth > 1) {
                if (root->length == 1) {
                    // have to call copy constructor here on pointer or else assignment to variant destroys root
                    // (and therefore destroys root->pointers[0]) before copy can happen
                    tree.rootVariant = NodePtr(root->pointers[0]);
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

        DoReplace(TreeType& tree, NodePtr<NodeType>& root) noexcept : tree(tree), root(root) {}

        template <typename ReplaceType>
        void operator()(ReplaceType&& replace) {
            bool do_collapse = false;
            if (replace.delta.ptrChanged) {
                if constexpr (NodeType::depth > 1) {
                    if (replace.delta.ptr->length == 1) {
                        do_collapse = true;
                    }
                }
                tree.rootVariant = std::move(replace.delta.ptr);
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

        DoSplit(TreeType& tree, NodePtr<NodeType>& root, uint64_t& iter) noexcept : tree(tree), root(root), iter(iter) {}

        template <typename SplitType>
        void operator()([[maybe_unused]] SplitType&& split) {
            if constexpr (NodeType::depth < maxDepth) {
                using NewRootType = InternalNode<NodeType::depth + 1>;
                auto rootNode = makePtr<NewRootType>();
                if (!split.left.ptrChanged) {
                    split.left.ptr = std::move(root);
                    split.left.ptrChanged = true;
                }
                rootNode->setElement(0, split.left);
                rootNode->setElement(1, split.right);
                rootNode->length = 2;
                rootNode->setIndex(iter, split.new_element_left ? 0 : 1);
                tree.rootVariant = std::move(rootNode);
            } else {
                throw std::logic_error("maximum depth exceeded");
            }
        }
    };

    template <typename TreeType>
    struct DoErase {
        TreeType& tree;

        explicit DoErase(TreeType& tree) noexcept : tree(tree) {}

        void operator()() {
            tree.rootVariant = makePtr<LeafNode>();
        }
    };

    template <typename Operation>
    struct Modify {
        template <typename TreeType, typename NodeType, typename F, typename T, typename... Us>
        uint64_t operator()(TreeType& tree, NodePtr<NodeType>& root, F&& finder, T const& searchVal, Us&&... params) {
            uint64_t ret = 0;
            Operation()(*root, searchVal, finder,
                DoReplace<TreeType, NodeType>(tree, root),
                DoSplit<TreeType, NodeType>(tree, root, ret),
                DoErase<TreeType>(tree),
                tree.treeSize,
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
