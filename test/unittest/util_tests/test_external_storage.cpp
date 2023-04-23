#include <cstdint>
#include <utility>

#include <gtest/gtest.h>

#include <trc_util/data/ExternalStorage.h>
using namespace trc::data;

struct MyData
{
    auto operator<=>(const MyData&) const = default;

    int a{ 0 };
    std::pair<float, int64_t> pair{ 0.0f, 0 };
};

TEST(ExternalStorageTest, DefaultConstruct)
{
    ExternalStorage<MyData> data0;
    ExternalStorage<MyData> data1;

    ASSERT_NE(data0.getDataId(), data1.getDataId());

    ASSERT_EQ(data0.get(), data1.get());
    ASSERT_EQ(data0.get().a, 0);
    ASSERT_FLOAT_EQ(data0.get().pair.first, 0.0f);
    ASSERT_EQ(data0.get().pair.second, 0);
    ASSERT_EQ(data1.get().a, 0);
    ASSERT_FLOAT_EQ(data1.get().pair.first, 0.0f);
    ASSERT_EQ(data1.get().pair.second, 0);

    data0.set({ 42, { -3.321f, -73 } });
    ASSERT_NE(data0.get(), data1.get());
    ASSERT_EQ(data0.get().a, 42);
    ASSERT_FLOAT_EQ(data0.get().pair.first, -3.321f);
    ASSERT_EQ(data0.get().pair.second, -73);
    ASSERT_EQ(data1.get().a, 0);
    ASSERT_FLOAT_EQ(data1.get().pair.first, 0.0f);
    ASSERT_EQ(data1.get().pair.second, 0);
}

TEST(ExternalStorageTest, GetDataFromID)
{
    using ID = ExternalStorage<MyData>::ID;

    ExternalStorage<MyData> data0;
    ExternalStorage<MyData> data1;

    data0.set({ 42, { 5.6f, -13 } });
    data1.set({ 73, { 3.14159f, 400 } });

    {
        const ID a = data0.getDataId();
        const ID b = data0.getDataId();
        ASSERT_EQ(a, b);
        ASSERT_EQ(a.get(), b.get());
        ASSERT_EQ(a.get(), data0.get());
        ASSERT_EQ(a.get(), ExternalStorage<MyData>::getData(a));

        const ID c = data1.getDataId();
        ASSERT_NE(a, c);
        ASSERT_NE(a.get(), c.get());
    }

    ASSERT_THROW(ID{}.get(), std::invalid_argument);
    ASSERT_THROW(ExternalStorage<MyData>::getData(ID{}), std::invalid_argument);
}

TEST(ExternalStorageTest, CopyAndMoveOperations)
{
    ExternalStorage<MyData> data0;
    data0.set({ 42, { 0.2f, 12 } });

    // Copy construct
    ExternalStorage<MyData> data1{ data0 };
    ASSERT_EQ(data1.get(), data0.get());

    // Move construct
    ExternalStorage<MyData> data2{ std::move(data0) };
    ASSERT_EQ(data2.get(), data1.get());

    // Copy assignment
    data2.set({ 3, { 4.5f, 6 } });
    data0 = data2;
    ASSERT_NE(data2.get(), data1.get());
    ASSERT_EQ(data2.get(), data0.get());

    // Move assignment
    data0 = std::move(data1);
    ASSERT_NE(data2.get(), data0.get());
}
