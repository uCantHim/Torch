#include <componentlib/StableTableImpl.h>

#define TABLE_IMPL_TEMPLATE componentlib::StableTableImpl
#define TABLE_TEST_NAME StableTableTest

#include "table_test_common.h"



struct Bar
{
    int i{ 0 };
    float f{ 2.71828f };
};

TEST(TABLE_TEST_NAME, UniqueStorage)
{
    static_assert(std::same_as<Bar,      Table<Bar>::value_type>);
    static_assert(std::same_as<Bar&,     Table<Bar>::reference>);
    static_assert(std::same_as<Bar*,     Table<Bar>::pointer>);
    static_assert(std::same_as<uint32_t, Table<Bar>::key_type>);

    Table<Bar> table;

    std::vector<Bar*> ptrs;
    ptrs.emplace_back(&table.emplace(0));
    ptrs.emplace_back(&table.emplace(1));
    ptrs.emplace_back(&table.emplace(42));
    ptrs.emplace_back(&table.emplace(6));

    table.erase(1);
    ptrs.erase(ptrs.begin() + 1);

    for (Bar* bar : ptrs)
    {
        ASSERT_TRUE(bar != nullptr);
        ASSERT_EQ(bar->i, 0);
        ASSERT_FLOAT_EQ(bar->f, 2.71828f);
    }
}
