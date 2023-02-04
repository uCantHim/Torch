#include <limits>
#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include <trc_util/data/OptionalStorage.h>

using trc::data::OptionalStorage;

static_assert(OptionalStorage<int, 0>::kSize == 0);
static_assert(OptionalStorage<int, 4>::kSize == 4);
static_assert(OptionalStorage<int, 200>::kSize == 200);

TEST(OptionalStorageTest, BasicValidity)
{
    OptionalStorage<int, 20> vec;

    for (size_t i = 0; i < vec.size(); ++i) {
        ASSERT_FALSE(vec.valid(i));
    }

    ASSERT_THROW(vec.valid(vec.size()), std::out_of_range);
    ASSERT_THROW(vec.valid(vec.size() + 5), std::out_of_range);
    ASSERT_THROW(vec.valid(vec.size() + 1234), std::out_of_range);
    ASSERT_THROW(vec.valid(std::numeric_limits<decltype(vec)::size_type>::max()),
                 std::out_of_range);

    vec.emplace(2, 42);
    for (size_t i = 0; i < vec.size(); ++i)
    {
        if (i == 2) {
            ASSERT_TRUE(vec.valid(i));
            ASSERT_NO_THROW(vec.at(i));
        }
        else {
            ASSERT_FALSE(vec.valid(i));
            ASSERT_THROW(vec.at(i), trc::data::InvalidElementAccess);
        }
    }
}

TEST(OptionalStorageTest, EmplaceErase)
{
    OptionalStorage<int, 50> vec;

    for (size_t i = 0; i < vec.size(); ++i) {
        ASSERT_EQ(i * 2, vec.emplace(i, i * 2));
    }
    for (size_t i = 0; i < vec.size(); ++i) {
        ASSERT_TRUE(vec.erase(i));
    }
    for (size_t i = 0; i < vec.size(); ++i) {
        ASSERT_FALSE(vec.valid(i));
    }

    ASSERT_THROW(vec.erase(vec.size()), std::out_of_range);
    ASSERT_THROW(vec.erase(vec.size() + 42), std::out_of_range);

    ASSERT_FALSE(vec.erase(15));
    ASSERT_NO_THROW(vec.emplace(15));
    ASSERT_NO_THROW(vec.emplace(15));
    ASSERT_TRUE(vec.erase(15));
    ASSERT_NO_THROW(vec.emplace(15));
}

TEST(OptionalStorageTest, TryEmplace)
{
    OptionalStorage<double, 10> vec;

    for (size_t i = 0; i < vec.size(); ++i)
    {
        auto [el, success] = vec.try_emplace(i, 42.5);
        ASSERT_TRUE(success);
        ASSERT_EQ(el, 42.5);
    }
    for (size_t i = 0; i < vec.size(); ++i)
    {
        auto [el, success] = vec.try_emplace(i, -12345);
        ASSERT_FALSE(success);
        ASSERT_EQ(el, 42.5);
    }
}

TEST(OptionalStorageTest, CorrectElementDestruction)
{
    struct Foo
    {
        Foo(int* count) : destructorCount(count) {}
        ~Foo() {
            *destructorCount += 1;
        }

        int* destructorCount;
    };

    OptionalStorage<Foo, 20> vec;

    // Emplace does not destroy anything (copy, move, ...)
    int count{ 0 };
    vec.emplace(3, &count);
    ASSERT_EQ(count, 0);

    // Erase correctly calls the destructor
    vec.erase(3);
    ASSERT_EQ(count, 1);
    vec.erase(3);
    ASSERT_EQ(count, 1);

    // Emplace into previously occupied location does not destroy the element twice
    int count2{ 0 };
    vec.emplace(3, &count2);
    ASSERT_EQ(count, 1);
    ASSERT_EQ(count2, 0);

    // Emplace into same location destroys existing element
    vec.emplace(3, &count2);
    ASSERT_EQ(count2, 1);
    vec.emplace(3, &count2);
    ASSERT_EQ(count2, 2);

    // try_emplace does not destroy existing element and does not create the new element
    vec.try_emplace(3, &count2);
    ASSERT_EQ(count2, 2);
}

TEST(OptionalStorageTest, Clear)
{
    OptionalStorage<int, 45> vec;

    vec.emplace(4, 20);
    vec.emplace(18, 7);
    vec.emplace(42, 0);
    ASSERT_TRUE(vec.valid(4));
    ASSERT_TRUE(vec.valid(18));
    ASSERT_TRUE(vec.valid(42));
    ASSERT_EQ(vec.at(4), 20);
    ASSERT_EQ(vec.at(18), 7);
    ASSERT_EQ(vec.at(42), 0);

    vec.clear();
    ASSERT_FALSE(vec.valid(4));
    ASSERT_FALSE(vec.valid(18));
    ASSERT_FALSE(vec.valid(42));

    struct Foo
    {
        Foo(int* count) : count(count) {}
        ~Foo() noexcept { *count += 1; }

        int* count;
    };

    // Test clear at end of scope
    int destructorCount{ 0 };
    {
        OptionalStorage<Foo, 20> fooVec;
        fooVec.emplace(0, &destructorCount);
        ASSERT_EQ(destructorCount, 0);
        fooVec.emplace(11, &destructorCount);
        ASSERT_EQ(destructorCount, 0);
        fooVec.emplace(12, &destructorCount);
        ASSERT_EQ(destructorCount, 0);
    } // end of scope

    ASSERT_EQ(destructorCount, 3);
}

TEST(OptionalStorageTest, CopyOperations)
{
    OptionalStorage<std::string, 50> src;
    OptionalStorage<std::string, 50> dst;

    src.emplace(0, "Hello");
    src.emplace(1, "World");
    dst.emplace(1, "Foo");
    dst.emplace(2, "Bar");

    dst = src;
    ASSERT_EQ(src.at(0), "Hello");
    ASSERT_EQ(src.at(1), "World");
    ASSERT_FALSE(src.valid(2));
    ASSERT_EQ(dst.at(0), "Hello");
    ASSERT_EQ(dst.at(1), "World");
    ASSERT_FALSE(dst.valid(2));

    // Prepare state
    for (size_t i = 0; i < src.size(); ++i)
    {
        src.emplace(i, std::to_string(i * 2));
        if (i % 2 == 0) {
            dst.erase(i);
        }
        else {
            dst.emplace(i, std::to_string(i));
        }
    }

    // Self-assignment
    src = src;
    for (size_t i = 0; i < src.size(); ++i) {
        ASSERT_EQ(src.at(i), std::to_string(i * 2));
    }

    dst = src;
    const OptionalStorage<std::string, 50> copyConstructed(src);
    for (size_t i = 0; i < src.size(); ++i)
    {
        ASSERT_EQ(src.at(i), std::to_string(i * 2));
        ASSERT_EQ(dst.at(i), src.at(i));
        ASSERT_EQ(copyConstructed.at(i), src.at(i));
    }

    src.clear();
    dst = src;
    for (size_t i = 0; i < src.size(); ++i)
    {
        ASSERT_FALSE(src.valid(i));
        ASSERT_FALSE(dst.valid(i));
        ASSERT_EQ(copyConstructed.at(i), std::to_string(i * 2));
    }
}

TEST(OptionalStorageTest, MoveConstructor)
{
    // Move constructor with regular type
    {
        OptionalStorage<int, 10> src;
        for (size_t i = 0; i < src.size(); ++i) {
            src.emplace(i, i - 4);
        }

        const OptionalStorage<int, 10> dst(std::move(src));
        for (size_t i = 0; i < src.size(); ++i)
        {
            ASSERT_FALSE(src.valid(i));
            ASSERT_EQ(dst.at(i), i - 4);
        }
    }

    // Move constructor with moveable type
    {
        OptionalStorage<std::string, 30> src;

        // Construct from empty storage
        OptionalStorage<std::string, 30> empty(std::move(src));
        for (size_t i = 0; i < src.size(); ++i)
        {
            ASSERT_FALSE(src.valid(i));
            ASSERT_FALSE(empty.valid(i));
        }

        // Prepare state
        for (size_t i = 0; i < src.size(); ++i) {
            src.emplace(i, std::to_string(i % 7));
        }

        OptionalStorage<std::string, 30> dst(std::move(src));
        for (size_t i = 0; i < src.size(); ++i)
        {
            ASSERT_FALSE(src.valid(i));
            ASSERT_TRUE(dst.valid(i));
            ASSERT_EQ(dst.at(i), std::to_string(i % 7));
        }
    }

    // Move constructor with move-only type
    {
        OptionalStorage<std::unique_ptr<int>, 20> src;
        for (size_t i = 0; i < src.size(); ++i) {
            src.emplace(i, new int(i - 4));
        }

        OptionalStorage<std::unique_ptr<int>, 20> dst(std::move(src));
        for (size_t i = 0; i < src.size(); ++i)
        {
            ASSERT_FALSE(src.valid(i));
            ASSERT_TRUE(dst.valid(i));
            ASSERT_EQ(*dst.at(i), i - 4);
        }
    }
}

TEST(OptionalStorageTest, MoveAssignment)
{
    OptionalStorage<std::string, 120> src;

    for (size_t i = 0; i < src.size(); ++i)
    {
        if (i % 3 == 0) {
            src.emplace(i, "Hello World!");
        }
    }

    // Self-assignment
    src = std::move(src);
    for (size_t i = 0; i < src.size(); ++i)
    {
        if (i % 3 == 0)
        {
            ASSERT_TRUE(src.valid(i));
            ASSERT_EQ(src.at(i), "Hello World!");
        }
        else {
            ASSERT_FALSE(src.valid(i));
        }
    }

    OptionalStorage<std::string, 120> dst;
    for (size_t i = 0; i < dst.size(); ++i)
    {
        if (i % 2 == 0) {
            dst.emplace(i);
        }
        else {
            dst.emplace(i, "Foo");
        }
    }

    dst = std::move(src);
    for (size_t i = 0; i < src.size(); ++i)
    {
        ASSERT_FALSE(src.valid(i));
        ASSERT_EQ(src.accessUnsafe(i), ""); // Assure that the unique_ptr has been moved
        if (i % 3 == 0) {
            ASSERT_EQ(dst.at(i), "Hello World!");
        }
        else {
            ASSERT_FALSE(dst.valid(i));
        }
    }

    // Assign again from the now-empty src
    dst = std::move(src);
    for (size_t i = 0; i < src.size(); ++i)
    {
        ASSERT_FALSE(src.valid(i));
        ASSERT_FALSE(dst.valid(i));
    }
}
