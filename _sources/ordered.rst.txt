.. _api_ordered:

Ordered
=============

Most of the time when using Ordered you will want the value type to be a std::pair. Ordered can support other value
types, such as std::tuple (using KeyValueExtractor = TupleExtractor) and even arbitrary user defined classes, but they
require a carefully crafted KeyValueExtractor to work properly.

.. doxygenstruct:: bpptree::detail::Ordered
   :project: B++ Tree
