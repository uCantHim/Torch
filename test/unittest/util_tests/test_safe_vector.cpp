#include <algorithm>
#include <atomic>
#include <future>
#include <numeric>
#include <random>
#include <ranges>
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

    ASSERT_THROW(vec.at(0), trc::data::InvalidElementAccess);
    ASSERT_NO_THROW(vec.at(2));

    ASSERT_TRUE(vec.erase(2));
    ASSERT_FALSE(vec.erase(2));

    ASSERT_NO_THROW(vec.emplace(15));
    ASSERT_NO_THROW(vec.emplace(15));
    ASSERT_TRUE(vec.erase(15));
    ASSERT_NO_THROW(vec.emplace(15));
}

TEST(SafeVectorTest, TryEmplace)
{
    SafeVector<double> vec;

    ASSERT_FLOAT_EQ(vec.emplace(0, 4.3), 4.3);
    ASSERT_FLOAT_EQ(vec.emplace(1, 4.3), 4.3);
    ASSERT_FLOAT_EQ(vec.emplace(2, 4.3), 4.3);
    ASSERT_FLOAT_EQ(vec.emplace(5, 4.3), 4.3);

    auto [a, s] = vec.try_emplace(4, -17);
    ASSERT_FLOAT_EQ(a, -17);
    ASSERT_TRUE(s);
    ASSERT_FALSE(vec.try_emplace(4, 23).second);
    ASSERT_FALSE(vec.try_emplace(5, 0).second);
    ASSERT_FALSE(vec.try_emplace(0, 89.67).second);
    ASSERT_FLOAT_EQ(vec.try_emplace(4, -1).first, -17);
    ASSERT_FLOAT_EQ(vec.try_emplace(5, -1).first, 4.3);
    ASSERT_FLOAT_EQ(vec.try_emplace(0, -1).first, 4.3);

    ASSERT_TRUE(vec.erase(0));
    ASSERT_TRUE(vec.try_emplace(0, 89.67).second);
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
    ASSERT_NO_THROW(vec.erase(3));
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

TEST(SafeVectorTest, ConcurrentEmplaceErase)
{
    constexpr size_t kNumElems{ 200000 };
    constexpr size_t kChunkSize{ 200 };
    constexpr size_t kNumThreads{ 10 };

    std::random_device dev;
    SafeVector<std::string, kChunkSize> vec;

    // Generate random index ranges
    std::array<std::array<size_t, kNumElems>, kNumThreads / 2> randomIndices;
    for (size_t i = 0; i < kNumThreads / 2; ++i)
    {
        auto& indices = randomIndices[i];
        std::iota(indices.begin(), indices.end(), 0);
        std::ranges::shuffle(indices, std::mt19937{ dev() });
    }

    auto emplacer = [&](auto begin, auto end) {
        for (auto it = begin; it != end; ++it) {
            vec.emplace(*it, "foo+bar");
        }
    };

    auto eraser = [&]{
        std::mt19937 rgen(dev());
        std::uniform_int_distribution<size_t> dist(0, kNumElems);

        std::this_thread::sleep_for(std::chrono::nanoseconds(1));
        for (int i = 0; i < kNumElems; ++i) {
            vec.erase(dist(rgen));
        }
    };

    std::vector<std::future<void>> futures;
    for (size_t i = 0; i < kNumThreads; ++i)
    {
        auto& indices = randomIndices.at(i / 2);
        futures.emplace_back(std::async(
            emplacer,
            indices.begin() + (i % 2) * (kNumElems / 2),
            indices.begin() + ((i % 2) + 1) * (kNumElems / 2)
        ));
        futures.emplace_back(std::async(eraser));
    }

    for (auto& fut : futures) {
        fut.wait();
    }
}

TEST(SafeVectorTest, Iterator)
{
    constexpr size_t kNumElems = 200000;
    constexpr size_t kChunkSize = 50;

    SafeVector<int, kChunkSize> vec;
    vec.reserve(kNumElems);

    // Fill vector
    for (size_t i = 0; i < kNumElems; ++i) {
        vec.emplace(i, 42);
    }

    // Non-const iterator
    int sum{ 0 };
    for (int& elem : vec) sum += elem;

    // Const iterator
    int csum{ 0 };
    for (const int& elem : std::as_const(vec)) csum += elem;

    // cbegin, cend
    ASSERT_EQ(vec.cbegin(), std::as_const(vec).begin());
    ASSERT_EQ(vec.cend(), std::as_const(vec).end());
    int csum2{ 0 };
    for (auto it = vec.cbegin(); it != vec.cend(); it++) {
        csum2 += *it;
    }

    const int truth = static_cast<int>(kNumElems) * 42;
    ASSERT_EQ(truth, sum);
    ASSERT_EQ(truth, csum);
}

TEST(SafeVectorTest, IteratorDeadlock)
{
    constexpr size_t kNumElems{ 10000 };

    SafeVector<int> vec;
    for (int i = 1; i <= kNumElems; ++i) {
        vec.emplace(i, i);
    }

    // No deadlock occurs when accessing the vector while iterating over it
    for (int i : vec)
    {
        if (i % 2 == 0)
        {
            vec.erase(i);
            vec.emplace(i - 1, 444);
        }
    }

    const int truth{ kNumElems / 2 * 444 };
    int sum{ 0 };
    for (int i : vec) sum += i;

    ASSERT_EQ(sum, truth);
}

TEST(SafeVectorTest, AtomicAccess)
{
    constexpr size_t kNumElems{ 20000 };
    constexpr size_t kNumThreads{ 10 };

    using T = std::array<uint64_t, 300>;
    SafeVector<T> vec;

    std::random_device dev;
    std::atomic<bool> terminate{ false };

    auto writer = [&](uint64_t val) {
        std::mt19937 rgen(dev());
        std::uniform_int_distribution<size_t> dist(0, kNumElems);
        while (!terminate)
        {
            const size_t index = dist(rgen);
            vec.applyAtomically(index, [val](T& arr) {
                for (uint64_t& i : arr) i = val;
            });
        }
    };

    ASSERT_THROW(vec.copyAtomically(42), std::out_of_range);
    ASSERT_THROW(vec.applyAtomically(42, [](T&) {}), std::out_of_range);
    ASSERT_THROW(std::as_const(vec).applyAtomically(42, [](const T&) {}), std::out_of_range);

    // Fill vector
    for (size_t i = 0; i < kNumElems; ++i) {
        vec.emplace(i);
    }

    ASSERT_NO_THROW(vec.copyAtomically(42));
    ASSERT_NO_THROW(vec.applyAtomically(42, [](T&) {}));
    ASSERT_EQ(vec.applyAtomically(42, [](T&) { return "foo"; }), "foo");
    ASSERT_EQ(
        std::as_const(vec).applyAtomically(
            42,
            [](const T&) { return std::make_pair(2, 1); }
        ),
        std::make_pair(2, 1)
    );

    // Multiple concurrent writers
    std::vector<std::future<void>> futures;
    for (size_t i = 0; i < kNumThreads; ++i) {
        futures.emplace_back(std::async(writer, i * 3));
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    terminate = true;
    for (auto& f : futures) f.wait();

    // Ensure that arrays were never written partially
    for (const T& arr : vec)
    {
        for (uint64_t val : arr) {
            ASSERT_EQ(val, arr[0]);
        }
    }

    // Again, this time with atomic copies
    futures.clear();
    terminate = false;

    std::mt19937 rgen(dev());
    std::uniform_int_distribution<uint64_t> dist(0, kNumElems);

    for (size_t i = 0; i < kNumThreads; ++i) {
        futures.emplace_back(std::async(writer, dist(rgen) * 2));
    }

    futures.emplace_back(std::async([&]{
        while (!terminate)
        {
            const size_t index = dist(rgen);
            T copy = vec.copyAtomically(index);

            // Array is uniform
            for (uint64_t val : copy) ASSERT_EQ(val, copy[0]);
        }
    }));

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    terminate = true;
    for (auto& f : futures) f.wait();
}
