#include <gtest/gtest.h>

#define FLAG_COMBINATION_NAMESPACE
#include "../auxiliary/FlagCombination.h"

enum class Foo
{
    eNone = 0,
    eOne,
    eTwo,
    eMaxEnum,
};

enum class Bar
{
    eNone = 0,
    eFirst = 1,
    eSecond = 2,
    eMaxEnum,
};

enum class Baz
{
    eNone = 0,
    a,
    b,
    c,
    eMaxEnum,
};

TEST(FlagCombinationTest, ConstexprTests)
{
    constexpr FlagCombination<Bar, Baz> a;
    static_assert(a.has(Bar::eNone));
    static_assert(!a.has(Bar::eFirst));
    static_assert(!a.has(Bar::eSecond));
    static_assert(a.toIndex() == 0);
    static_assert(a & Baz::eNone);
    static_assert(!(a & Baz::c));

    constexpr auto flags = Bar::eFirst | Baz::b;
    static_assert(!flags.has(Baz::a));
    static_assert(flags.has(Bar::eFirst));
    static_assert(flags.has(Baz::b));
    static_assert(flags.toIndex() == 7);

    // Copy from smaller type
    constexpr FlagCombination<Bar, Baz, Foo> b(flags);
    static_assert(b.has(Bar::eFirst));
    static_assert(b.has(Baz::b));
    static_assert(b.has(Foo::eNone));

    constexpr auto flags2 = flags | Foo::eTwo;
    static_assert(flags2.has(Bar::eFirst));
    static_assert(flags2.has(Baz::b));
    static_assert(flags2.has(Foo::eTwo));
    static_assert(flags2.toIndex() == 31);

    constexpr auto max = Bar::eSecond | Baz::c;
    static_assert(max.toIndex() == max.size() - 1);
}
