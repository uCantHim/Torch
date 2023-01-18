#include <algorithm>
#include <numeric>
#include <random>
#include <ranges>

#include <gtest/gtest.h>

#include <trc_util/data/DeferredInsertVector.h>
using trc::data::DeferredInsertVector;

TEST(DeferredInsertVectorTest, BasicUsage)
{
    DeferredInsertVector<float> vec;
    ASSERT_TRUE(vec.empty());
    ASSERT_EQ(vec.size(), 0);

    ASSERT_THROW(vec.at(0), std::out_of_range);
    ASSERT_THROW(vec.at(-1), std::out_of_range);
    ASSERT_THROW(vec.at(1), std::out_of_range);
    ASSERT_THROW(vec.at(876), std::out_of_range);

    vec.update();
    ASSERT_TRUE(vec.empty());
    ASSERT_EQ(vec.size(), 0);

    vec.emplace_back(3.14159f);
    vec.update();
    ASSERT_FALSE(vec.empty());
    ASSERT_EQ(vec.size(), 1);
    ASSERT_FLOAT_EQ(vec.at(0), 3.14159f);
}

TEST(DeferredInsertVectorTest, ComplexInsertErase)
{
    DeferredInsertVector<int> vec;

    vec.emplace_back(2);
    vec.emplace_back(0);
    vec.emplace_back(42);
    {
        auto range = vec.iter();
        vec.erase(std::ranges::find(range, 0));
    }
    vec.update();
    ASSERT_FALSE(vec.empty());
    ASSERT_EQ(vec.size(), 2);
    ASSERT_NO_THROW(vec.at(0));
    ASSERT_NO_THROW(vec.at(1));
    ASSERT_THROW(vec.at(2), std::out_of_range);
    ASSERT_EQ(vec.at(0), 2);
    ASSERT_EQ(vec.at(1), 42);
}

TEST(DeferredInsertVectorTest, Iterate)
{
    DeferredInsertVector<int> vec;
    for (int i = 0; i < 120; ++i) {
        vec.emplace_back(i);
    }
    vec.update();

    for (int i = 0; int val : vec.iter())
    {
        ASSERT_EQ(val, i);
        ++i;
    }
}

TEST(DeferredInsertVectorTest, MultipleErase)
{
    constexpr size_t kNumElems{ 10000 };
    constexpr size_t kNumErasedElems{ 5000 };

    std::vector<int> truth(kNumElems);
    std::iota(truth.begin(), truth.end(), 0);

    DeferredInsertVector<int> vec;
    vec.reserve(kNumElems);
    for (size_t i = 0; i < kNumElems; ++i) {
        vec.emplace_back(i);
    }

    std::vector<size_t> indices(kNumErasedElems);
    std::iota(indices.begin(), indices.end(), 0);

    std::random_device dev;
    std::mt19937 en(dev());
    std::ranges::shuffle(indices, en);

    // Force deferred erase by acquiring lock
    for (auto range = vec.iter(); size_t i : indices) {
        vec.erase(range.begin() + i);
    }

    // Prepare ground truth
    std::ranges::sort(indices, std::greater<int>{});
    for (size_t i : indices) {
        truth.erase(truth.begin() + i);
    }

    vec.update();
    ASSERT_FALSE(vec.empty());
    ASSERT_EQ(vec.size(), kNumElems - kNumErasedElems);
    ASSERT_EQ(vec.size(), truth.size());
    for (size_t i = 0; i < kNumElems - kNumErasedElems; ++i)
    {
        ASSERT_EQ(vec.at(i), truth.at(i));
    }
}
