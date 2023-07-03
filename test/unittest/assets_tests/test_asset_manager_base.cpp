#include <gtest/gtest.h>

#include <trc/assets/AssetManagerBase.h>

#include "assets_tests/memory_data_storage.h"
#include "define_asset_type.h"

DEFINE_ASSET_TYPE(DummyAsset, DummyRegistry);
using DummyID = trc::TypedAssetID<DummyAsset>;
using DummyData = trc::AssetData<DummyAsset>;

DEFINE_ASSET_TYPE(Foo, FooRegistry);
using FooID = trc::TypedAssetID<Foo>;
using FooData = trc::AssetData<Foo>;

///////////////////////////////////////
///     Declare the test class      ///
///////////////////////////////////////

class AssetManagerBaseTest : public testing::Test
{
protected:
    AssetManagerBaseTest()
    {
        assets.getDeviceRegistry().addModule<DummyAsset>(std::make_unique<DummyRegistry>());
        assets.getDeviceRegistry().addModule<Foo>(std::make_unique<FooRegistry>());
    }

    trc::AssetManagerBase assets;
};

struct MyAssetSource : public trc::AssetSource<DummyAsset>
{
public:
    explicit MyAssetSource(std::string_view name) : name(name) {}

    auto load() -> DummyData override
    {
        return {
            .number=42,
            .string="The answer"
        };
    }

    auto getMetadata() -> trc::AssetMetadata override
    {
        return {
            .name=std::string{ name },
            .type=trc::AssetType::make<DummyAsset>(),
            .path=std::nullopt,
        };
    }

    std::string_view name;
};

inline auto path(std::string_view str) -> trc::AssetPath {
    return trc::AssetPath(str);
}

TEST_F(AssetManagerBaseTest, InvalidAssetIdError)
{
    ASSERT_NO_THROW(trc::InvalidAssetIdError(0));
    ASSERT_NO_THROW(trc::InvalidAssetIdError(0, ""));
    ASSERT_NO_THROW(trc::InvalidAssetIdError(42, ""));
    ASSERT_NO_THROW(trc::InvalidAssetIdError(std::numeric_limits<ui32>::max(), ""));
    ASSERT_NO_THROW(trc::InvalidAssetIdError(3, "hello"));
    ASSERT_NO_THROW(trc::InvalidAssetIdError(1234567, "not empty anymore :o"));
}

TEST_F(AssetManagerBaseTest, CreateAsset)
{
    auto id = assets.create<DummyAsset>(std::make_unique<MyAssetSource>("foo"));
    ASSERT_EQ(&id.getAssetManager(), &assets);
    ASSERT_NO_THROW(id.getDeviceDataHandle());
    ASSERT_EQ(id.getMetadata().name, "foo");
    ASSERT_EQ(id.getMetadata().type, trc::AssetType::make<DummyAsset>());
    ASSERT_EQ(id.getMetadata().path, std::nullopt);

    // AssetManagerBase::getMetadata
    ASSERT_EQ(assets.getMetadata(id).name, id.getMetadata().name);
    ASSERT_EQ(assets.getMetadata(id).type, id.getMetadata().type);
    ASSERT_EQ(assets.getMetadata(id).path, id.getMetadata().path);

    trc::AssetID typeless = id.getAssetID();
    ASSERT_TRUE(assets.getAs<DummyAsset>(typeless));
}

TEST_F(AssetManagerBaseTest, DestroyAsset)
{
    auto foo = assets.create<DummyAsset>(std::make_unique<MyAssetSource>("foo"));
    auto bar = assets.create<DummyAsset>(std::make_unique<MyAssetSource>("bar"));
    auto baz = assets.create<DummyAsset>(std::make_unique<MyAssetSource>("foo"));

    ASSERT_NO_THROW(assets.getMetadata(foo));
    ASSERT_NO_THROW(assets.getMetadata(bar));
    ASSERT_NO_THROW(assets.getMetadata(baz));

    assets.destroy(bar);
    ASSERT_THROW(assets.destroy(bar),                          trc::InvalidAssetIdError);
    ASSERT_THROW(assets.destroy<DummyAsset>(bar.getAssetID()), trc::InvalidAssetIdError);
    ASSERT_THROW(assets.getMetadata(bar),                      trc::InvalidAssetIdError);
    ASSERT_THROW(assets.getHandle(bar),                        trc::InvalidAssetIdError);
    ASSERT_THROW(assets.getAssetType(bar),                     trc::InvalidAssetIdError);
    ASSERT_THROW(assets.getAs<DummyAsset>(bar.getAssetID()),   trc::InvalidAssetIdError);

    // foo still exists
    ASSERT_NO_THROW(assets.getMetadata(foo));
    ASSERT_NO_THROW(assets.getHandle(foo));
    ASSERT_NO_THROW(assets.getAssetType(foo));
    ASSERT_TRUE(assets.getAs<DummyAsset>(foo.getAssetID()));

    // Try the other `destroy` signature
    trc::AssetID typelessFoo = foo.getAssetID();
    assets.destroy<DummyAsset>(typelessFoo);
    ASSERT_THROW(assets.destroy(foo),                        trc::InvalidAssetIdError);
    ASSERT_THROW(assets.destroy<DummyAsset>(typelessFoo),    trc::InvalidAssetIdError);
    ASSERT_THROW(assets.getMetadata(foo),                    trc::InvalidAssetIdError);
    ASSERT_THROW(assets.getHandle(foo),                      trc::InvalidAssetIdError);
    ASSERT_THROW(assets.getAssetType(foo),                   trc::InvalidAssetIdError);
    ASSERT_THROW(assets.getAs<DummyAsset>(foo.getAssetID()), trc::InvalidAssetIdError);

    ASSERT_NO_THROW(assets.destroy(baz));
}

TEST_F(AssetManagerBaseTest, IterAssets)
{
    static_assert(std::is_const_v<std::remove_reference_t<decltype(*assets.begin())>>);

    ui32 count{ 0 };

    for (auto& _ : assets) {
        ++count;
    }
    ASSERT_EQ(count, 0);

    assets.create<DummyAsset>(std::make_unique<MyAssetSource>("hello world"));
    assets.create<Foo>(std::make_unique<trc::InMemorySource<Foo>>(FooData{}));
    assets.create<DummyAsset>(std::make_unique<trc::InMemorySource<DummyAsset>>(DummyData{}));
    for (auto& asset : assets)
    {
        switch(count)
        {
        case 0:
            ASSERT_TRUE(asset.asType<DummyAsset>());
            ASSERT_FALSE(asset.getMetadata().path);
            break;
        case 1:
            ASSERT_TRUE(asset.asType<Foo>());
            ASSERT_FALSE(asset.getMetadata().path);
            break;
        case 2:
            ASSERT_TRUE(asset.asType<DummyAsset>());
            ASSERT_FALSE(asset.getMetadata().path);
            break;
        }
        ++count;
    }
    ASSERT_EQ(count, 3);
}
