#include <string>

#include <gtest/gtest.h>

#include <trc_util/data/PropertyStack.h>
using namespace trc::data;

enum class Foo
{
    firstname,
    lastname,
    age,
    email,
    foo_value,
    data
};

struct Thing
{
    int i{ 0 };
};

using Field = PropertyField<Foo, std::string, std::string, int, std::string, double, Thing>;
using Stack = PropertyStack<Foo, std::string, std::string, int, std::string, double, Thing>;

TEST(TestPropertyStack, Internals)
{
    using Tuple = std::tuple<int32_t, double, std::string, std::byte>;
    static_assert(internal::offset<0, Tuple> == 0);
    static_assert(internal::offset<1, Tuple> == 4);
    static_assert(internal::offset<2, Tuple> == 12);
    static_assert(internal::offset<3, Tuple> == 12 + sizeof(std::string));
    static_assert(internal::offset<4, Tuple> == 12 + sizeof(std::string) + 1);
    static_assert(internal::offset<4, Tuple> == internal::elem_size_sum<3, Tuple>);
}

TEST(TestPropertyStack, PropertyField)
{
    Field field;

    ASSERT_FALSE(field.has<Foo::firstname>());
    ASSERT_FALSE(field.has<Foo::lastname>());
    ASSERT_FALSE(field.has<Foo::age>());
    ASSERT_FALSE(field.has<Foo::email>());
    ASSERT_FALSE(field.has<Foo::foo_value>());
    ASSERT_FALSE(field.has<Foo::data>());
    ASSERT_THROW(field.get<Foo::firstname>(), std::out_of_range);
    ASSERT_THROW(field.get<Foo::lastname>(),  std::out_of_range);
    ASSERT_THROW(field.get<Foo::age>(),       std::out_of_range);
    ASSERT_THROW(field.get<Foo::email>(),     std::out_of_range);
    ASSERT_THROW(field.get<Foo::foo_value>(), std::out_of_range);
    ASSERT_THROW(field.get<Foo::data>(),      std::out_of_range);

    field.set<Foo::firstname>("Hello");
    field.set<Foo::lastname>("World");
    field.set<Foo::age>(999);
    field.set<Foo::email>("hello.world@foo.bar");
    field.set<Foo::foo_value>(3.1415926);
    field.set<Foo::data>(Thing{ .i=42 });

    ASSERT_TRUE(field.has<Foo::firstname>());
    ASSERT_TRUE(field.has<Foo::lastname>());
    ASSERT_TRUE(field.has<Foo::age>());
    ASSERT_TRUE(field.has<Foo::email>());
    ASSERT_TRUE(field.has<Foo::foo_value>());
    ASSERT_TRUE(field.has<Foo::data>());
    ASSERT_NO_THROW(field.get<Foo::firstname>());
    ASSERT_NO_THROW(field.get<Foo::lastname>());
    ASSERT_NO_THROW(field.get<Foo::age>());
    ASSERT_NO_THROW(field.get<Foo::email>());
    ASSERT_NO_THROW(field.get<Foo::foo_value>());
    ASSERT_NO_THROW(field.get<Foo::data>());
}

TEST(TestPropertyStack, EmptyPropertyThrows)
{
    Stack stack;

    ASSERT_THROW(stack.top<Foo::firstname>(), std::out_of_range);
    ASSERT_THROW(stack.top<Foo::lastname>(),  std::out_of_range);
    ASSERT_THROW(stack.top<Foo::age>(),       std::out_of_range);
    ASSERT_THROW(stack.top<Foo::email>(),     std::out_of_range);
    ASSERT_THROW(stack.top<Foo::foo_value>(), std::out_of_range);
    ASSERT_THROW(stack.top<Foo::data>(),      std::out_of_range);

    ASSERT_THROW(stack.pop<Foo::firstname>(), std::out_of_range);
    ASSERT_THROW(stack.pop<Foo::lastname>(),  std::out_of_range);
    ASSERT_THROW(stack.pop<Foo::age>(),       std::out_of_range);
    ASSERT_THROW(stack.pop<Foo::email>(),     std::out_of_range);
    ASSERT_THROW(stack.pop<Foo::foo_value>(), std::out_of_range);
    ASSERT_THROW(stack.pop<Foo::data>(),      std::out_of_range);
}

TEST(TestPropertyStack, PushPop)
{
    Stack stack;

    stack.push<Foo::age>(42);
    stack.push<Foo::foo_value>(2.718281828);
    stack.push<Foo::lastname>("Dude");

    ASSERT_EQ(stack.top<Foo::lastname>(), "Dude");
    ASSERT_EQ(stack.top<Foo::age>(), 42);
    ASSERT_FLOAT_EQ(stack.top<Foo::foo_value>(), 2.718281828);
    ASSERT_THROW(stack.top<Foo::firstname>(), std::out_of_range);
    ASSERT_THROW(stack.top<Foo::email>(),     std::out_of_range);
    ASSERT_THROW(stack.top<Foo::data>(),      std::out_of_range);

    ASSERT_THROW(stack.pop<Foo::firstname>(), std::out_of_range);
    ASSERT_THROW(stack.pop<Foo::email>(),     std::out_of_range);
    ASSERT_THROW(stack.pop<Foo::data>(),      std::out_of_range);

    ASSERT_NO_THROW(stack.pop<Foo::lastname>());
    ASSERT_NO_THROW(stack.pop<Foo::age>());
    ASSERT_NO_THROW(stack.pop<Foo::foo_value>());
    ASSERT_THROW(stack.pop<Foo::lastname>(),  std::out_of_range);
    ASSERT_THROW(stack.pop<Foo::age>(),       std::out_of_range);
    ASSERT_THROW(stack.pop<Foo::foo_value>(), std::out_of_range);
}

TEST(TestPropertyStack, MultiPush)
{
    Stack stack;

    stack.push<Foo::age>(42);
    stack.push<Foo::age>(8);
    stack.push<Foo::age>(12345);

    ASSERT_EQ(stack.top<Foo::age>(), 12345);
    stack.pop<Foo::age>();
    ASSERT_EQ(stack.top<Foo::age>(), 8);
    stack.pop<Foo::age>();

    stack.push<Foo::age>(0);
    ASSERT_EQ(stack.top<Foo::age>(), 0);
    stack.pop<Foo::age>();
    ASSERT_EQ(stack.top<Foo::age>(), 42);
    stack.pop<Foo::age>();

    ASSERT_THROW(stack.top<Foo::age>(), std::out_of_range);
    ASSERT_THROW(stack.pop<Foo::age>(), std::out_of_range);
}

TEST(TestPropertyStack, ConstructFromField)
{
    Field field;
    field.set<Foo::data>(Thing{ .i=42 });
    field.set<Foo::email>("hello.world@foo.bar");

    Stack stack(field);
    ASSERT_THROW(stack.pop<Foo::firstname>(), std::out_of_range);
    ASSERT_THROW(stack.pop<Foo::lastname>(),  std::out_of_range);
    ASSERT_THROW(stack.pop<Foo::age>(),       std::out_of_range);
    ASSERT_THROW(stack.pop<Foo::foo_value>(), std::out_of_range);
    ASSERT_NO_THROW(stack.pop<Foo::email>());
    ASSERT_NO_THROW(stack.pop<Foo::data>());
}
