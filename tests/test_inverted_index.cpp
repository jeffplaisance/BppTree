#include "gtest/gtest.h"
#include "test_common.hpp"
#include "inverted_index.hpp"

TEST(BppTreeTest, TestInvertedIndex) {
    InvertedIndex<uint32_t, 512, 512, 2>::Transient index{};
    index.insert(5u, 0u);
    index.insert(8u, 0u);
    index.insert(8u, 2u);
    index.insert(2u, 2u);
    index.insert(3u, 1u);
    index.insert(2u, 1u);
    index.insert(5u, 1u);
    auto term_it = index.term_list().begin();
    auto term_end = index.term_list().end();
    ASSERT_NE(term_it, term_end);
    // verify term 2
    ASSERT_EQ(term_it->first, 2u);
    ASSERT_EQ(term_it->second, 2u);
    {
        auto [begin, end] = index.term_doc_iterator(term_it);
        ASSERT_NE(begin, end);
        ASSERT_EQ(*begin, 1u);
        ASSERT_NE(++begin, end);
        ASSERT_EQ(*begin, 2u);
        ASSERT_EQ(++begin, end);
    }
    ASSERT_NE(++term_it, term_end);
    // verify term 3
    ASSERT_EQ(term_it->first, 3u);
    ASSERT_EQ(term_it->second, 1u);
    {
        auto [begin, end] = index.term_doc_iterator(term_it);
        ASSERT_NE(begin, end);
        ASSERT_EQ(*begin, 1u);
        ASSERT_EQ(++begin, end);
    }
    ASSERT_NE(++term_it, term_end);
    // verify term 5
    ASSERT_EQ(term_it->first, 5u);
    ASSERT_EQ(term_it->second, 2u);
    {
        auto [begin, end] = index.term_doc_iterator(term_it);
        ASSERT_NE(begin, end);
        ASSERT_EQ(*begin, 0u);
        ASSERT_NE(++begin, end);
        ASSERT_EQ(*begin, 1u);
        ASSERT_EQ(++begin, end);
    }
    ASSERT_NE(++term_it, term_end);
    // verify term 8
    ASSERT_EQ(term_it->first, 8u);
    ASSERT_EQ(term_it->second, 2u);
    {
        auto [begin, end] = index.term_doc_iterator(term_it);
        ASSERT_NE(begin, end);
        ASSERT_EQ(*begin, 0u);
        ASSERT_NE(++begin, end);
        ASSERT_EQ(*begin, 2u);
        ASSERT_EQ(++begin, end);
    }
    ASSERT_EQ(++term_it, term_end);
}
