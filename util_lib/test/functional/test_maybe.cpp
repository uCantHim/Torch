#include <cmath>
#include <string>
#include <vector>

#include <gtest/gtest.h>
#include <nc/functional/Maybe.h>

namespace nc {
    using namespace functional;
}

struct Foo
{
    double d{ 420.187 };
    std::string str;
    std::vector<int> vec{ 1, 2, 3 };
};

TEST(MaybeTest, Get)
{
    ASSERT_EQ(nc::Maybe<int32_t>{ 0 }.get(), 0);
    ASSERT_EQ(nc::Maybe<int32_t>{ 1 }.get(), 1);
    ASSERT_EQ(nc::Maybe<int32_t>{ -1 }.get(), -1);
    ASSERT_EQ(nc::Maybe<int32_t>{ 42 }.get(), 42);
    ASSERT_EQ(nc::Maybe<int32_t>{ -187 }.get(), -187);
    ASSERT_EQ(nc::Maybe<int32_t>{ INT32_MIN     }.get(), INT32_MIN);
    ASSERT_EQ(nc::Maybe<int32_t>{ INT32_MAX     }.get(), INT32_MAX);

    // Access to empty Maybe must throw
    nc::Maybe<int> m;
    nc::Maybe<Foo> m_foo;

    ASSERT_THROW(m.get(), nc::MaybeEmptyError);
    ASSERT_THROW(m_foo.get(), nc::MaybeEmptyError);
}

TEST(MaybeTest, GetOr)
{
    nc::Maybe<int32_t> value{ 42 };
    nc::Maybe<int32_t> empty;

    ASSERT_EQ(value.getOr(0), 42);
    ASSERT_EQ(empty.getOr(42), 42);
}

TEST(MaybeTest, MonadicPipeOperator)
{
    nc::Maybe<int>{ 42 } >> [](int i) { ASSERT_EQ(i, 42); };
    ASSERT_EQ(
        (nc::Maybe<int>{ 42 } >> [](int i) { return i * 2; }).get(),
        84
    );

    nc::Maybe<float>{ 2.718 } >> [](float f) { ASSERT_FLOAT_EQ(f, 2.718); };
    ASSERT_FLOAT_EQ(
        (nc::Maybe<float>{ 2.718 } >> [](float f) { return floor(f); }).get(),
        2.0f
    );

    nc::Maybe<int> mEmpty;
    nc::Maybe<Foo> mEmptyFoo;

    // Empty Maybe into lambda does not call lambda
    ASSERT_NO_THROW(mEmpty >> [](int i) { ASSERT_TRUE(false); });
    ASSERT_NO_THROW(mEmptyFoo >> [](Foo foo) { ASSERT_TRUE(false); });

    // Empty Maybe into returning lambda does not throw but returns nothing
    ASSERT_NO_THROW(mEmpty >> [](int i) { return i + 4; });
    ASSERT_NO_THROW(mEmptyFoo >> [](Foo foo) { return foo; });
}

TEST(MaybeTest, OrOperatorWithVariable)
{
    int resultI = 7;

    nc::Maybe<int> m{ resultI };
    nc::Maybe<int> empty;

    ASSERT_EQ(m || 800, 7);
    ASSERT_EQ(empty || 67, 67);
}
