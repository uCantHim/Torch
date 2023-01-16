#include <future>
#include <string>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

#include <trc_util/data/SafeVector.h>

using trc::util::SafeVector;

TEST(SafeVectorTest, BasicValidityTests)
{
    SafeVector<int> vec;

    ASSERT_FALSE(vec.contains(0));
    ASSERT_FALSE(vec.contains(1));
    ASSERT_FALSE(vec.contains(22));
    ASSERT_FALSE(vec.contains(1234));

    vec.emplace(2, 42);
    ASSERT_FALSE(vec.contains(0));
    ASSERT_FALSE(vec.contains(1));
    ASSERT_TRUE(vec.contains(2));
    ASSERT_FALSE(vec.contains(3));
    ASSERT_FALSE(vec.contains(4));
    ASSERT_FALSE(vec.contains(1234));

    ASSERT_THROW(vec.at(0), std::out_of_range);
    ASSERT_NO_THROW(vec.at(2));

    ASSERT_NO_THROW(vec.erase(2));
    ASSERT_THROW(vec.erase(2), std::out_of_range);

    ASSERT_NO_THROW(vec.emplace(15));
    ASSERT_NO_THROW(vec.emplace(15));
    ASSERT_NO_THROW(vec.erase(15));
    ASSERT_NO_THROW(vec.emplace(15));
}

TEST(SafeVectorTest, AutoResize)
{
    SafeVector<double, 20> vec;

    ASSERT_NO_THROW(vec.emplace(0, 0));    // Chunk 0
    ASSERT_NO_THROW(vec.emplace(20, 0));   // Chunk 1
    ASSERT_NO_THROW(vec.emplace(113, 14)); // Chunk 6
    ASSERT_NO_THROW(vec.emplace(70, 0));   // Chunk 4

    ASSERT_EQ(vec.at(113), 14);
}

TEST(SafeVectorTest, CorrectDestruction)
{
    struct Foo
    {
        Foo(int* count) : destructorCount(count) {}
        ~Foo() {
            *destructorCount += 1;
        }

        int* destructorCount;
    };

    SafeVector<Foo> vec;

    // Emplace does not destroy anything (copy, move, ...)
    int count{ 0 };
    vec.emplace(3, &count);
    ASSERT_EQ(count, 0);

    // Erase correctly calls the destructor
    vec.erase(3);
    ASSERT_EQ(count, 1);
    ASSERT_THROW(vec.erase(3), std::out_of_range);
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
}

TEST(SafeVectorTest, MemorySafety)
{
    struct Foo
    {
        int i{ 0 };
    };
    constexpr size_t kChunkSize{ 50 };

    SafeVector<Foo, kChunkSize> vec;

    vec.emplace(4,  Foo{ 42 });
    vec.emplace(38, Foo{ -3210 });
    Foo& ref1 = vec.at(4);
    Foo& ref2 = vec.at(38);
    ASSERT_EQ(ref1.i, 42);
    ASSERT_EQ(ref2.i, -3210);

    vec.emplace(kChunkSize, Foo{ 800 });
    ASSERT_EQ(ref1.i, 42);
    ASSERT_EQ(ref2.i, -3210);
    ASSERT_EQ(vec.at(kChunkSize).i, 800);
}

TEST(SafeVectorTest, ThreadsafeResize)
{
    constexpr size_t kChunkSize{ 5 };
    constexpr size_t kNumThreads{ 60 };
    constexpr size_t kIterationsPerThread{ 2000 };

    auto kernel = [](SafeVector<std::string, kChunkSize>& vec, size_t offset)
    {
        for (size_t i = 0; i < kIterationsPerThread; ++i)
        {
            const size_t chunk = i * kNumThreads + offset;
            vec.emplace(chunk * kChunkSize);
        }
    };

    SafeVector<std::string, kChunkSize> vec;

    std::vector<std::future<void>> futures;
    futures.reserve(kNumThreads);
    for (size_t i = 0; i < kNumThreads; ++i) {
        futures.emplace_back(std::async(kernel, std::ref(vec), i));
    }
    for (auto& f : futures) f.wait();

    for (size_t i = 0; i < kIterationsPerThread * kNumThreads; ++i)
    {
        ASSERT_TRUE(vec.contains(i * kChunkSize + 0));
        ASSERT_FALSE(vec.contains(i * kChunkSize + 1));
        ASSERT_FALSE(vec.contains(i * kChunkSize + 2));
        ASSERT_FALSE(vec.contains(i * kChunkSize + 3));
        ASSERT_FALSE(vec.contains(i * kChunkSize + 4));
    }
}

TEST(SafeVectorTest, Clear)
{
    SafeVector<int> vec;

    vec.emplace(4, 20);
    vec.emplace(18, 7);
    vec.emplace(42, 0);
    ASSERT_TRUE(vec.contains(4));
    ASSERT_TRUE(vec.contains(18));
    ASSERT_TRUE(vec.contains(42));
    ASSERT_EQ(vec.at(4), 20);
    ASSERT_EQ(vec.at(18), 7);
    ASSERT_EQ(vec.at(42), 0);

    vec.clear();
    ASSERT_FALSE(vec.contains(4));
    ASSERT_FALSE(vec.contains(18));
    ASSERT_FALSE(vec.contains(42));

    struct Foo
    {
        Foo(int* count) : count(count) {}
        ~Foo() noexcept { *count += 1; }

        int* count;
    };

    int destructorCount{ 0 };

    {
        SafeVector<Foo> fooVec;
        fooVec.emplace(0, &destructorCount);
        ASSERT_EQ(destructorCount, 0);
        fooVec.clear();
        ASSERT_EQ(destructorCount, 1);
    } // end of scope
    ASSERT_EQ(destructorCount, 1);
}
