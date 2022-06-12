#include <vector>

#include <gtest/gtest.h>

#include "VariantFlagSet.h"

/// NOLINTNEXTLINE
TEST(VariantFlagSetTest, SortedAfterListConstruction)
{
    VariantFlagSet set{ { 1, 0 }, { 3, 1 }, { 2, 1 }, { 0, 4 } };

    ASSERT_FALSE(set.empty());
    ASSERT_EQ(set.size(), 4);

    std::vector<VariantFlag> value{ set.begin(), set.end() };
    std::vector<VariantFlag> truth{ { 0, 4 }, { 1, 0 }, { 2, 1 }, { 3, 1 } };
    ASSERT_EQ(value, truth);
}

/// NOLINTNEXTLINE
TEST(VariantFlagSetTest, SortedAfterEmplace)
{
    VariantFlagSet set;
    ASSERT_TRUE(set.empty());

    for (auto flag : std::vector<VariantFlag>{ { 1, 0 }, { 3, 1 }, { 2, 1 }, { 0, 4 } })
    {
        set.emplace(flag);
    }
    ASSERT_FALSE(set.empty());
    ASSERT_EQ(set.size(), 4);

    std::vector<VariantFlag> value{ set.begin(), set.end() };
    std::vector<VariantFlag> truth{ { 0, 4 }, { 1, 0 }, { 2, 1 }, { 3, 1 } };
    ASSERT_EQ(value, truth);
}

/// NOLINTNEXTLINE
TEST(VariantFlagSetTest, DuplicateListConstructionThrows)
{
    std::initializer_list<VariantFlag> list{ { 0, 1 }, { 0, 2 } };
    std::initializer_list<VariantFlag> list2{ { 3, 3 }, { 4, 0 }, { 0, 1 }, { 4, 2 } };
    ASSERT_THROW(VariantFlagSet{ list }, std::invalid_argument);
    ASSERT_THROW(VariantFlagSet{ list2 }, std::invalid_argument);
}

/// NOLINTNEXTLINE
TEST(VariantFlagSetTest, DuplicateEmplaceThrows)
{
    VariantFlagSet set;
    ASSERT_NO_THROW(set.emplace({ 6, 0 }));
    ASSERT_NO_THROW(set.emplace({ 3, 4 }));
    ASSERT_THROW(set.emplace({ 3, 0 }), std::invalid_argument);
}

/// NOLINTNEXTLINE
TEST(VariantFlagSetTest, ContainsFlagType)
{
    VariantFlagSet set{ { 6, 0 }, { 7, 4 }, { 2, 1 }, { 5, 4 }, { 0, 0 } };

    ASSERT_TRUE(set.contains(6));
    ASSERT_TRUE(set.contains(7));
    ASSERT_TRUE(set.contains(2));
    ASSERT_TRUE(set.contains(5));
    ASSERT_TRUE(set.contains(0));

    ASSERT_FALSE(set.contains(1));
    ASSERT_FALSE(set.contains(3));
    ASSERT_FALSE(set.contains(4));
    ASSERT_FALSE(set.contains(8));
    ASSERT_FALSE(set.contains(4444444));
    ASSERT_FALSE(set.contains(std::numeric_limits<size_t>::max()));
}

/// NOLINTNEXTLINE
TEST(VariantFlagSetTest, ContainsFlagBit)
{
    VariantFlagSet set{ { 6, 0 }, { 7, 4 }, { 2, 1 }, { 5, 4 }, { 0, 0 } };

    ASSERT_TRUE(set.contains({ 6, 0 }));
    ASSERT_TRUE(set.contains({ 7, 4 }));
    ASSERT_TRUE(set.contains({ 2, 1 }));
    ASSERT_TRUE(set.contains({ 5, 4 }));
    ASSERT_TRUE(set.contains({ 0, 0 }));

    ASSERT_FALSE(set.contains({ 6, 1 }));
    ASSERT_FALSE(set.contains({ 7, 2 }));
    ASSERT_FALSE(set.contains({ 2, 0 }));
    ASSERT_FALSE(set.contains({ 5, 11 }));
    ASSERT_FALSE(set.contains({ 0, 2 }));
    ASSERT_FALSE(set.contains({ 8, 0 }));
    ASSERT_FALSE(set.contains({ 2345, 2 }));
    ASSERT_FALSE(set.contains({ std::numeric_limits<size_t>::max(), 0 }));
    ASSERT_FALSE(set.contains({ 0, std::numeric_limits<size_t>::max() }));
    ASSERT_FALSE(set.contains({ std::numeric_limits<size_t>::max(), std::numeric_limits<size_t>::max() }));
}
