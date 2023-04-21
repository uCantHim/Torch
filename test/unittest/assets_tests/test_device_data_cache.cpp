#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <gtest/gtest.h>

#include <trc/assets/DeviceDataCache.h>
using namespace trc::basic_types;

struct MyData
{
    MyData(std::string str, int64_t i) : num(i), string(std::move(str)) {}

    MyData(MyData&&) noexcept = default;
    ~MyData() noexcept = default;

    MyData(const MyData&) = delete;
    MyData& operator=(const MyData&) = delete;
    MyData& operator=(MyData&&) noexcept = delete;

    int64_t num;
    std::string string;
};

auto makeCache(auto load, auto free) -> trc::DeviceDataCache<MyData>
{
    return trc::DeviceDataCache<MyData>{
        trc::DeviceDataCache<MyData>::makeLoader(std::move(load), std::move(free))
    };
}

TEST(DeviceDataCacheTest, BasicDataLoad)
{
    auto cache = makeCache(
        [](ui32 id){ return MyData{ "Hello cache!", i64{ id } }; },
        [](ui32 id, MyData data){}
    );

    auto a = cache.get(3);
    auto b = cache.get(0);
    auto c = cache.get(42);

    ASSERT_EQ(a->num, 3);
    ASSERT_EQ(b->num, 0);
    ASSERT_EQ(c->num, 42);
    ASSERT_EQ(a->string, "Hello cache!");
    ASSERT_EQ(b->string, "Hello cache!");
    ASSERT_EQ(c->string, "Hello cache!");
}

TEST(DeviceDataCacheTest, DataFreedWhenHandleDestroyed)
{
    std::unordered_map<ui32, bool> exists;

    auto cache = makeCache(
        [&](ui32 id){
            exists[id] = true;
            return MyData{ "Hello cache!", i64{ id } };
        },
        [&](ui32 id, MyData data){ exists[id] = false; }
    );

    std::optional<decltype(cache)::CacheEntryHandle> handles[4]{
        cache.get(5),
        cache.get(14),
        cache.get(2),
        cache.get(321),
    };
    std::unordered_set<ui32> ids{ 5, 14, 2, 321 };

    {
        auto& a = *handles[0];
        auto& b = *handles[1];
        auto& c = *handles[2];
        auto& d = *handles[3];

        ASSERT_EQ(a->num, 5);
        ASSERT_EQ(b->num, 14);
        ASSERT_EQ(c->num, 2);
        ASSERT_EQ(d->num, 321);

        for (ui32 i = 0; i < 1000; ++i)
        {
            ASSERT_EQ(exists.contains(i), ids.contains(i));
            ASSERT_EQ(exists[i], ids.contains(i));
        }
    }

    handles[1].reset();
    ids.erase(14);
    for (ui32 i = 0; i < 1000; ++i) {
        ASSERT_EQ(exists[i], ids.contains(i));
    }

    handles[2].reset();
    ids.erase(2);
    for (ui32 i = 0; i < 1000; ++i) {
        ASSERT_EQ(exists[i], ids.contains(i));
    }

    handles[0].reset();
    handles[3].reset();
    for (ui32 i = 0; i < 1000; ++i) {
        ASSERT_FALSE(exists[i]);
    }
}

TEST(DeviceDataCacheTest, MultipleHandlesToSameObject)
{
    bool exists{ false };
    ui32 numCreates{ 0 };
    ui32 numFrees{ 0 };

    auto cache = makeCache(
        [&](ui32 id) {
            exists = true;
            ++numCreates;
            return MyData{ "I am a friendly string :)", i64{ id } };
        },
        [&](ui32 id, MyData data) {
            exists = false;
            ++numFrees;
        }
    );

    std::vector<decltype(cache)::CacheEntryHandle> handles;

    ASSERT_FALSE(exists);
    ASSERT_EQ(numCreates, 0);
    ASSERT_EQ(numFrees, 0);

    handles.emplace_back(cache.get(0));
    ASSERT_TRUE(exists);
    ASSERT_EQ(numCreates, 1);
    ASSERT_EQ(numFrees, 0);

    // Copy the same handle
    for (int i = 0; i < 34; ++i) {
        handles.emplace_back(handles.front());
    }
    ASSERT_TRUE(exists);
    ASSERT_EQ(numCreates, 1);
    ASSERT_EQ(numFrees, 0);

    // Free all copied handles
    handles.clear();
    ASSERT_FALSE(exists);
    ASSERT_EQ(numCreates, 1);
    ASSERT_EQ(numFrees, 1);

    numCreates = 0;
    numFrees = 0;

    // Create multiple new handles via `get`
    for (int i = 0; i < 246; ++i)
    {
        if (i % 5 == 4) {
            handles.emplace_back(handles.back());
        }
        else {
            handles.emplace_back(cache.get(0));
        }
    }
    ASSERT_TRUE(exists);
    ASSERT_EQ(numCreates, 1);
    ASSERT_EQ(numFrees, 0);

    // Erase some of the handles
    handles.erase(handles.begin() + 4, handles.begin() + 53);
    ASSERT_TRUE(exists);
    ASSERT_EQ(numCreates, 1);
    ASSERT_EQ(numFrees, 0);

    // Create one extra handle via copy, then let it go out of scope
    {
        auto h = handles.at(99);
        ASSERT_EQ(h->num, 0);
        ASSERT_EQ(h->string, "I am a friendly string :)");

        handles.clear();
        ASSERT_TRUE(exists);
        ASSERT_EQ(numCreates, 1);
        ASSERT_EQ(numFrees, 0);
    }
    ASSERT_FALSE(exists);
    ASSERT_EQ(numCreates, 1);
    ASSERT_EQ(numFrees, 1);
}
