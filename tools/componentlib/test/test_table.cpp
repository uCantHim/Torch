#include <vector>

#include <gtest/gtest.h>
#include <gmock/gmock.h>
namespace t = testing;

#include <componentlib/Table.h>
#include <componentlib/TableUtils.h>
using namespace componentlib;

auto createJoin(auto& a, auto& b)
{
    using A = typename std::decay_t<decltype(a)>::value_type;
    using B = typename std::decay_t<decltype(b)>::value_type;

    std::vector<std::pair<A, B>> result;
    join(a, b, [&result](auto& a, auto& b) {
        result.emplace_back(a, b);
    });

    return result;
}

auto createJoinIt(auto& a, auto& b)
{
    using A = typename std::decay_t<decltype(a)>::value_type;
    using B = typename std::decay_t<decltype(b)>::value_type;

    std::vector<std::pair<A, B>> result;
    for (auto [_, i, j] : a.join(b)) {
        result.emplace_back(i, j);
    }

    return result;
}



TEST(TableTest, KeyIterator)
{
    Table<int> table;
    table.emplace(4, 42);
    table.emplace(1, 187);
    table.emplace(3, 3);
    table.emplace(7, 9);
    table.emplace(8, 0);
    table.erase(3);
    table.erase(4);
    table.emplace(3, 20);

    std::vector<uint32_t> keys;
    for (auto it = table.keyBegin(); it != table.keyEnd(); it++) {
        keys.push_back(*it);
    }
    ASSERT_THAT(keys, testing::ElementsAre(1, 3, 7, 8));
}

TEST(TableTest, TwoTableJoin)
{
    Table<int> table;
    table.emplace(4, 42);
    table.emplace(1, 187);
    table.emplace(3, 3);
    table.emplace(7, 9);
    table.emplace(8, 0);
    table.erase(3);
    table.emplace(3, 20);

    // Second table
    Table<std::string> table2;
    table2.emplace(1, "Foo");
    table2.emplace(7, "Baz");
    table2.emplace(3, "Bar");

    auto joinResult = createJoin(table, table2);
    ASSERT_THAT(
        joinResult,
        t::ElementsAre(
            t::Pair(187, "Foo"),
            t::Pair(20, "Bar"),
            t::Pair(9, "Baz")
        )
    );
}

TEST(TableTest, JoinIterator)
{
    Table<int> table1;
    table1.emplace(4, 42);
    table1.emplace(1, 187);
    table1.emplace(3, 3);
    table1.emplace(7, 9);
    table1.emplace(8, 0);
    table1.erase(3);
    table1.emplace(3, 20);

    // Second table to join with first one
    Table<std::string> table2;
    table2.emplace(1, "Foo");
    table2.emplace(7, "Baz");
    table2.emplace(3, "Bar");

    std::vector<std::pair<int, std::string>> joinResult;
    for (auto [_, i, s] : table1.join(table2))
    {
        joinResult.emplace_back(i, s);
    }

    ASSERT_THAT(
        joinResult,
        t::ElementsAre(
            t::Pair(187, "Foo"),
            t::Pair(20, "Bar"),
            t::Pair(9, "Baz")
        )
    );

    // Reverse join
    joinResult.clear();
    for (auto [_, s, i] : table2.join(table1))
    {
        joinResult.emplace_back(i, s);
    }

    ASSERT_THAT(
        joinResult,
        t::ElementsAre(
            t::Pair(187, "Foo"),
            t::Pair(20, "Bar"),
            t::Pair(9, "Baz")
        )
    );
}

TEST(TableTest, JoinEmpty)
{
    Table<int> t1, t2;
    ASSERT_TRUE(createJoin(t1, t2).empty());

    std::vector<std::pair<int, int>> result;
    for (auto [_, i, j] : t1.join(t2)) {
        result.emplace_back(i, j);
    }
    ASSERT_TRUE(result.empty());
}

TEST(TableTest, JoinOneEmpty)
{
    Table<int> t1;
    Table<int> t2;
    t2.emplace(2, 50);
    t2.emplace(6, 100);
    t2.emplace(3, 150);

    ASSERT_TRUE(createJoinIt(t1, t2).empty());
    ASSERT_TRUE(createJoinIt(t2, t1).empty());
}

TEST(TableTest, ConstIteratorsCompileTime)
{
    const Table<int> t;

    for ([[maybe_unused]] auto k : t.keys()) {}
    for ([[maybe_unused]] auto v : t.values()) {}
    for ([[maybe_unused]] auto [k, v] : t.items()) {}
}

struct Bar
{
    int i{ 0 };
    float f{ 2.71828f };
};

template<>
struct componentlib::TableTraits<Bar>
{
    struct UniqueStorage{};
};

TEST(TableTest, UniqueStorage)
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
