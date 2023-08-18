#include <unordered_set>
#include <vector>

#include <gtest/gtest.h>
#include <gmock/gmock.h>
namespace t = testing;

#include <componentlib/Table.h>
#include <componentlib/TableUtils.h>

template<typename Value>
using Table = componentlib::Table<Value, uint32_t, TABLE_IMPL_TEMPLATE<Value, uint32_t>>;

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



TEST(TABLE_TEST_NAME, Iterator)
{
    constexpr int kNumElems = 3000;

    const std::unordered_set<std::string> set = []{
        std::unordered_set<std::string> set;
        for (int i = 0; i < kNumElems; ++i) {
            set.emplace("elem #" + std::to_string(i));
        }
        return set;
    }();

    Table<std::string> table;
    std::unordered_set<std::string> truth;
    for (int i = 0; const auto& str : set)
    {
        if (i % 2 == 0) {
            table.emplace(i, str);
            truth.emplace(str);
        }
        if (i % 5 == 0) {
            if (table.try_erase(i)) {
                truth.erase(str);
            }
        }
        ++i;
    }

    // Verify
    std::unordered_set<std::string> test;
    for (const std::string& str : table) {
        test.emplace(str);
    }

    ASSERT_EQ(truth, test);
}

TEST(TABLE_TEST_NAME, KeyIterator)
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
    for (auto it = table.keyBegin(); it != table.keyEnd(); ++it) {
        keys.push_back(*it);
    }
    ASSERT_THAT(keys, testing::ElementsAre(1, 3, 7, 8));
}

TEST(TABLE_TEST_NAME, PairIterator)
{
    constexpr int kNumElems = 2000;

    Table<std::string> table;
    for (int i = 0; i < kNumElems; ++i) {
        table.emplace(i, "elem #" + std::to_string(i));
    }
    for (int i = 0; i < kNumElems; ++i)
    {
        if (i % 2 == 1) {
            table.erase(i);
        }
    }

    int i = 0;
    for (const auto& [key, str] : table.items())
    {
        ASSERT_EQ(i * 2, key);
        ASSERT_EQ(str, "elem #" + std::to_string(i * 2));
        ASSERT_EQ(str, "elem #" + std::to_string(key));
        ++i;
    }

    ASSERT_EQ(i, kNumElems / 2);
}

TEST(TABLE_TEST_NAME, TwoTableJoin)
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

TEST(TABLE_TEST_NAME, JoinIterator)
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

TEST(TABLE_TEST_NAME, JoinEmpty)
{
    Table<int> t1, t2;
    ASSERT_TRUE(createJoin(t1, t2).empty());
    ASSERT_TRUE(createJoin(t2, t1).empty());
    ASSERT_TRUE(createJoinIt(t1, t2).empty());
    ASSERT_TRUE(createJoinIt(t2, t1).empty());
}

TEST(TABLE_TEST_NAME, JoinOneEmpty)
{
    Table<int> t1;
    Table<int> t2;
    t2.emplace(2, 50);
    t2.emplace(6, 100);
    t2.emplace(3, 150);

    ASSERT_TRUE(createJoinIt(t1, t2).empty());
    ASSERT_TRUE(createJoinIt(t2, t1).empty());
}

TEST(TABLE_TEST_NAME, ConstIteratorsCompileTime)
{
    const Table<int> t;

    for ([[maybe_unused]] auto k : t.keys()) {}
    for ([[maybe_unused]] auto v : t.values()) {}
    for ([[maybe_unused]] auto [k, v] : t.items()) {}
}
