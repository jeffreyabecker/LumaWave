#include <unity.h>

#include "core/IndexIterator.h"

namespace
{
void test_index_iterator_progresses_with_step(void)
{
    lw::IndexIterator it(10, 5, 3);
    const lw::IndexSentinel end{};

    TEST_ASSERT_FALSE(it == end);
    TEST_ASSERT_EQUAL_size_t(10, *it);
    ++it;
    TEST_ASSERT_EQUAL_size_t(15, *it);
    ++it;
    TEST_ASSERT_EQUAL_size_t(20, *it);
    ++it;
    TEST_ASSERT_TRUE(it == end);
}

void test_index_iterator_uses_size_t_without_uint8_wrap(void)
{
    lw::IndexIterator it(250, 10, 3);
    const lw::IndexSentinel end{};

    TEST_ASSERT_EQUAL_size_t(250, *it);
    ++it;
    TEST_ASSERT_EQUAL_size_t(260, *it);
    ++it;
    TEST_ASSERT_EQUAL_size_t(270, *it);
    ++it;
    TEST_ASSERT_TRUE(it == end);
}

void test_index_iterator_post_increment_returns_previous(void)
{
    lw::IndexIterator it(1, 2, 2);

    const auto before = it++;
    TEST_ASSERT_EQUAL_size_t(1, *before);
    TEST_ASSERT_EQUAL_size_t(3, *it);
}

void test_index_range_size_returns_configured_count(void)
{
    const lw::IndexRange range(10, 7, 4);

    TEST_ASSERT_EQUAL_size_t(4, range.size());
    TEST_ASSERT_EQUAL_size_t(6, lw::IndexRange::fromCount(6).size());
}
} // namespace

void setUp(void)
{
}

void tearDown(void)
{
}

int main(int, char**)
{
    UNITY_BEGIN();
    RUN_TEST(test_index_iterator_progresses_with_step);
    RUN_TEST(test_index_iterator_uses_size_t_without_uint8_wrap);
    RUN_TEST(test_index_iterator_post_increment_returns_previous);
    RUN_TEST(test_index_range_size_returns_configured_count);
    return UNITY_END();
}
