.. B++ Tree documentation master file, created by
   sphinx-quickstart on Thu Apr 20 21:24:04 2023.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

B++ Tree
=============

.. image:: https://github.com/jeffplaisance/BppTree/actions/workflows/workflow.yml/badge.svg
   :target: https://github.com/jeffplaisance/BppTree/actions?query=workflow%3ATests+branch%3Amain
   :alt: GitHub Actions Badge

.. image:: https://codecov.io/gh/jeffplaisance/BppTree/branch/main/graph/badge.svg
   :target: https://codecov.io/gh/jeffplaisance/BppTree
   :alt: CodeCov Badge

`See full documentation here <https://jeffplaisance.github.io/BppTree>`_

B++ Tree is a header only B+ tree library written in C++. B++ Trees can be mutable or immutable, and you can easily
convert between the two. B++ Trees also support a variety of mixins, each of which adds additional functionality to the
tree. You can add as many mixins to a B++ Tree as you want, enabling you to create things such as a map that can be
accessed by key or by index, or a vector that supports calculating the sum between any two indexes in O(log N) time.
A priority queue that can be accessed both by priority order and insertion order? Yep, B++ Trees can do that too!

B++ Trees have all the usual cache locality benefits of B+ trees that make them faster than balanced binary search
trees for most use cases. I have experimentally determined that 512 bytes is a good size for the nodes in a B++ Tree, so
that is what I have selected as the default, but the node size is parameterized and you can set it to whatever you'd
like.

The B++ Tree library requires C++17 or later.

Mutability
--------------

All B++ Trees are easily convertible between their mutable and immutable representations. In this library, the mutable
form of a B++ Tree is called Transient and the immutable form of a B++ Tree is called
`Persistent <https://en.wikipedia.org/wiki/Persistent_data_structure>`_. Every Transient tree
has a .persistent() method which returns a Persistent version of the tree, and every Persistent tree has a .transient()
method that returns a Transient version of the tree. Persistent trees are always copy on write. Any time the
.persistent() method is called on a Transient tree, the entire Transient tree will be marked copy on write, and future
modifications to the Transient tree will copy the nodes they would otherwise have to modify. The same applies when a
Transient tree is created by calling the .transient() method on a Persistent tree, any nodes that would be modified by
the Transient tree will first make a copy. In the worst case, making a Persistent tree from a Transient tree by calling
.persistent() is O(N) because it must mark every node copy on write. If you have a Persistent tree which you turn
into a Transient tree, modify, and then turn back into a Persistent tree, this will only be O(M log M) where M is the
number of modifications that were made.

Mixins
--------------------

The internal nodes of a B++ Tree with no mixins only contain pointers to their children. Without mixins, a B++ tree can
be used as a mutable or immutable replacement for ``std::deque`` (with O(log N) ``emplace_front`` and ``emplace_back``),
but that is about all it can do. Almost all of the functionality comes from the mixins, and as they are all optional,
you only have to pay for what you choose to use. The only restriction is that each mixin can only be added once to a
tree, otherwise the method names will conflict and you will only be able to call the methods for one of them.

Ordered
^^^^^^^^

:ref:`api_ordered` adds lookup by key into a B++ tree. Values in the B++ tree must be ordered by key to use Ordered.
Supports lookup, lower_bound, upper_bound, assign, insert, update, and erase by key in O(log N) time.

Indexed
^^^^^^^^

:ref:`api_indexed` adds support for indexing into a B++ tree by an integer index. Supports lookup, assign, insert,
update, and erase by index in O(log N) time. Also supports getting the index of an element from an iterator. Given two
iterators a and b from an indexed tree, ``a-b`` is O(log N) (it is O(N) for non-indexed trees)

Summed
^^^^^^^^^

:ref:`api_summed` adds prefix sum support to a B++ tree in O(log N) time. ``sum()`` returns the sum over the entire
tree. ``tree.sum_inclusive(it)`` returns the sum up to and including the element pointed to by the iterator 'it'.
``tree.sum_exclusive(it)`` returns the sum up to but not including the element pointed to by the iterator 'it'. If you
want to sum between two iterators, just do ``tree.sum_exclusive(it2)-tree.sum_exclusive(it1)``. ``tree.sum_lower_bound(target)``
returns an iterator pointing to the first element for which ``tree.sum_inclusive(it) >= target``.

Min
^^^^^

:ref:`api_min` adds support for finding the minimum value in a B++ tree in O(log N) time. You can find just the min
value using the ``tree.min()`` method or get an iterator pointing to the min element with ``tree.min_element()``. You
can also find the min value/element in a subrange of the tree (identified by two iterators) with ``tree.min(it1, it2)``
or ``tree.min_element(it1, it2)``.

Max
^^^^^

:ref:`api_max` adds support for finding the maximum value in a B++ tree in O(log N) time. You can find just the max
value using the ``tree.max()`` method or get an iterator pointing to the max element with ``tree.max_element()``. You
can also find the max value/element in a subrange of the tree (identified by two iterators) with ``tree.max(it1, it2)``
or ``tree.max_element(it1, it2)``. You may be wondering why there are separate mixins for min and max when the only
difference is the comparison function: it is so that you can use both in the same B++ Tree without creating method name
conflicts!

Iterator Stability
---------------------

Iterators into Persistent B++ Trees are never invalidated. Iterators into Transient B++ Trees are not invalidated when
modifying an existing element in the tree, i.e. the assign and update methods do not invalidate iterators. **ALL** iterators
into a Transient B++ Tree are invalidated whenever an insert or erase is performed, with the exception that the methods
``tree.erase(it)`` and ``tree.insert(it, value)`` do not invalidate the iterator that they are called with (but all other
iterators into tree would still be invalidated).

Proxy References
--------------------
The only way to allow assignment through ``operator[]`` on Ordered and Indexed is with proxy references, because the
metadata in the internal nodes of the B++ Tree has to be updated on all modifications. As a result, you should not use
auto to declare the type of a variable to which the result of ``operator[]`` is assigned. The non-const iterators also
use proxy references for ``operator*()`` for the same reason. If you do not like proxy references, there are named
methods (``at_*``, ``assign_*``, ``update_*``) which do the same things without using proxy references.

Builders
----------

Ok, enough about how B++ Trees work, how do I use one? The BppTree template is used to specify the value type
(required), and optionally the leaf and internal node sizes in bytes, as well as a maximum depth for the tree, which can
reduce compile times and code size if set to a smaller number at the expense of limiting the maximum tree size. The
BppTree template has a member called ``mixins``, which accepts any number of mixin builders. The mixin builders have
reasonable defaults for the most part, and you can override these defaults to customize them for your use case. Most
commonly, you might want to override the extractors, which are responsible for extracting a field from the value to use
with the mixin, so that for example you can have an Ordered and Summed B++ Tree with std::pair values that uses the
first element of the pair as the key and calculates sums over the second element (see SummingMap example below). If you
implement your own extractor, note that extractors must have a constructor that takes no arguments and must have no
mutable state.

Here is how to make a B++ Tree with no mixins:

.. code-block:: cpp

   using NoMixins = BppTree<int32_t>::Transient;
   NoMixins no_mixins{};
   no_mixins.emplace_back(5);
   no_mixins.emplace_front(2);
   assert(no_mixins.front() == 2);
   assert(no_mixins.back() == 5);

Here is how to make a B++ Tree that can be used like a ``std::map``:

.. code-block:: cpp

   using Map = BppTree<pair<int32_t, int32_t>>::mixins<OrderedBuilder<>>::Transient;
   Map bpptree_map{};
   bpptree_map[5] = 8;
   if (bpptree_map[5] == 8) {
       ++bpptree_map[5];
   }
   assert(bpptree_map[5] == 9);

Because Ordered B++ trees where the value is a key value pair are incredibly common, there is a convenience helper
which makes the above a little bit simpler:

.. code-block:: cpp

   using Map = BppTreeMap<int32_t, int32_t>::Transient;

Here is how to make a B++ Tree that can be used like a ``std::vector``:

.. code-block:: cpp

   using Vector = BppTree<int32_t>::mixins<IndexedBuilder<>>::Transient;
   Vector bpptree_vec{};
   bpptree_vec.emplace_back(5);
   if (bpptree_vec[0] == 5) {
       ++bpptree_vec[0];
   }
   assert(bpptree_vec[0] == 6);

As with the Ordered example above, there is also a convenience helper for making Indexed B++ trees:

.. code-block:: cpp

   using Vector = BppTreeVector<int32_t>::Transient;

Here is how to make a B++ Tree that can be used like a ``std::vector`` and that can calculate prefix sums in O(log N) time:

.. code-block:: cpp

   using SummingVector = BppTreeVector<int32_t>::mixins<SummedBuilder<>>::Transient;
   SummingVector bpptree_vec2{};
   bpptree_vec2.emplace_back(5);
   bpptree_vec2.emplace_back(2);
   if (bpptree_vec2[0] == 5) {
       ++bpptree_vec2[0];
   }
   assert(bpptree_vec2.sum() == 8);

Here is how to make a B++ Tree that can be used like a ``std::deque`` and a ``std::priority_queue`` at the same time:

.. code-block:: cpp

   using Queue = BppTree<int32_t>::mixins<MinBuilder<>>::Transient;
   Queue bpptree_queue{};
   bpptree_queue.emplace_back(5);
   bpptree_queue.emplace_back(2);
   bpptree_queue.emplace_back(3);
   assert(bpptree_queue.min() == 2);
   assert(bpptree_queue.front() == 5);
   assert(bpptree_queue.back() == 3);

Here is how to make a B++ Tree that can be used like a ``std::map`` and also calculate prefix sums in O(log N) time:

.. code-block:: cpp

   using SummingMap = BppTreeMap<int32_t, int32_t>::mixins<SummedBuilder<PairExtractor<1>>>::Transient;
   SummingMap bpptree_map{};
   bpptree_map[5] = 8;
   if (bpptree_map[5] == 8) {
       ++bpptree_map[5];
   }
   bpptree_map[8] = 4;
   bpptree_map[6] = 2;
   assert(bpptree_map.sum() == 15);
   assert(bpptree_map.sum_inclusive(bpptree_map.begin() + 1) == 11);

Here is how to make a B++ Tree that can be used like a ``std::set``

.. code-block:: cpp

   using TreeSet = BppTree<int32_t>::mixins<OrderedBuilder<>::extractor<ValueExtractor>>::Transient;
   TreeSet tree_set{};
   tree_set.insert_or_assign(5);
   tree_set.insert_or_assign(8);
   tree_set.insert_or_assign(5);
   assert(tree_set.contains(5));
   assert(tree_set.contains(8));
   assert(tree_set.size == 2);

Like BppTreeMap and BppTreeVector, there is also a convenience helper for making set-like B++ trees:

.. code-block:: cpp

   using TreeSet = BppTreeSet<int32_t>::Transient;

Take a look at examples/inverted_index.hpp for a neat implementation of an inverted index, complete with Transient and
Persistent variations, implemented using B++ Trees.

Interface Stability
-------------------

Long term, I would like to maintain a stable API for B++ Trees, but as this project is quite new, I reserve the
right to change the API in order to make the B++ Tree library better. That said, I am fairly happy with the API as it
stands today and have no current plans to change it.

Table of Contents
==================
.. toctree::
    :maxdepth: 2

    self
    api
