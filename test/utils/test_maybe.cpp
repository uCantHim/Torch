#include <string>
#include <vector>

#include <gtest/gtest.h>
#include <trc/utils/Maybe.h>

struct Foo
{
    double d{ 420.187 };
    std::string str;
    std::vector<int> vec{ 1, 2, 3 };
};

TEST(MaybeTest, CompileTimeTests)
{
    trc::Maybe<int> m;
    auto iFunc = [](int) {};

    static_assert(std::is_same_v<void, decltype(m >> iFunc)>, "");
}

TEST(MaybeTest, GetThrowsIfEmpty)
{
    trc::Maybe<int> m;
    trc::Maybe<Foo> m_foo;

    ASSERT_THROW(m.get(), trc::MaybeEmptyError);
    ASSERT_THROW(m_foo.get(), trc::MaybeEmptyError);
}

TEST(MaybeTest, GetOrReturnsCorrectValue)
{
    trc::Maybe<int32_t> value{ 42 };
    trc::Maybe<int32_t> empty;

    ASSERT_EQ(value.getOr(0), 42);
    ASSERT_EQ(empty.getOr(42), 42);
}

TEST(MaybeTest, GetReturnsValue)
{
    ASSERT_EQ(trc::Maybe<int32_t>{ 0 }.get(), 0);
    ASSERT_EQ(trc::Maybe<int32_t>{ 1 }.get(), 1);
    ASSERT_EQ(trc::Maybe<int32_t>{ -1 }.get(), -1);
    ASSERT_EQ(trc::Maybe<int32_t>{ 42 }.get(), 42);
    ASSERT_EQ(trc::Maybe<int32_t>{ -187 }.get(), -187);
    ASSERT_EQ(trc::Maybe<int32_t>{ INT32_MIN     }.get(), INT32_MIN);
    ASSERT_EQ(trc::Maybe<int32_t>{ INT32_MIN - 1 }.get(), INT32_MIN - 1);
    ASSERT_EQ(trc::Maybe<int32_t>{ INT32_MIN + 1 }.get(), INT32_MIN + 1);
    ASSERT_EQ(trc::Maybe<int32_t>{ INT32_MAX     }.get(), INT32_MAX);
    ASSERT_EQ(trc::Maybe<int32_t>{ INT32_MAX - 1 }.get(), INT32_MAX - 1);
    ASSERT_EQ(trc::Maybe<int32_t>{ INT32_MAX + 1 }.get(), INT32_MAX + 1);
}

TEST(MaybeTest, EmptyPipeIntoVariableThrows)
{
    int i;
    Foo foo;
    trc::Maybe<int> m;
    trc::Maybe<Foo> m_foo;

    ASSERT_THROW(m >> i,       trc::MaybeEmptyError);
    ASSERT_THROW(m_foo >> foo, trc::MaybeEmptyError);
}

TEST(MaybeTest, EmptyPipeIntoFunctionThrows)
{
    auto iFunc = [](int i) {};
    auto fooFunc = [](Foo foo) {};
    auto iFuncR = [](int i) { return i + 4; };
    auto fooFuncR = [](Foo foo) { return foo; };

    trc::Maybe<int> m;
    trc::Maybe<Foo> m_foo;

    ASSERT_THROW(m >> iFunc,        trc::MaybeEmptyError);
    ASSERT_THROW(m >> iFuncR,       trc::MaybeEmptyError);
    ASSERT_THROW(m_foo >> fooFunc,  trc::MaybeEmptyError);
    ASSERT_THROW(m_foo >> fooFuncR, trc::MaybeEmptyError);
}

TEST(MaybeTest, PipeOperatorIntoVariable)
{
    double resultD = 123.45;
    std::string resultStr = "Hello World!";
    std::vector<int> resultVec = { 9, 8, 7, 6, 5 };

    trc::Maybe<double> m{ resultD };
    trc::Maybe<Foo> m_foo{ { resultD, resultStr, resultVec } };

    double d;
    Foo foo;
    m >> d;
    m_foo >> foo;

    ASSERT_DOUBLE_EQ(d, resultD);
    ASSERT_DOUBLE_EQ(foo.d, resultD);
    ASSERT_EQ(foo.str, resultStr);
    ASSERT_EQ(foo.vec, resultVec);
}

TEST(MaybeTest, OrOperatorWithVariable)
{
    int resultI = 7;

    trc::Maybe<int> m{ resultI };
    trc::Maybe<int> empty;

    ASSERT_EQ(m || 800, 7);
    ASSERT_EQ(empty || 67, 67);
}
