#include <gtest/gtest.h>

#include <trc/assets/AssetManager.h>
#include <trc/assets/AssetReference.h>
#include <trc/util/FilesystemDataStorage.h>
#include <trc_util/MemoryStream.h>

#include "assets_tests/memory_data_storage.h"
#include "define_asset_type.h"
#include "test_utils.h"

DEFINE_ASSET_TYPE(DummyAsset, DummyRegistry);
using DummyID = trc::TypedAssetID<DummyAsset>;
using DummyData = trc::AssetData<DummyAsset>;

class AssetReferenceTest : public testing::Test
{
protected:
    AssetReferenceTest()
        :
        manager(std::make_shared<MemoryStorage>())
    {
        manager.registerAssetType<DummyAsset>(std::make_unique<DummyRegistry>());
    }

    trc::AssetManager manager;
};

TEST_F(AssetReferenceTest, EmptyReference)
{
    trc::AssetReference<DummyAsset> ref;

    ASSERT_TRUE(ref.empty());
    ASSERT_FALSE(ref.hasAssetPath());
    ASSERT_FALSE(ref.hasResolvedID());
    ASSERT_THROW(ref.getAssetPath(), std::runtime_error);
    ASSERT_THROW(ref.getID(), std::runtime_error);
}

TEST_F(AssetReferenceTest, FromAssetPath)
{
    trc::AssetPath path("my/asset/file.txt");
    trc::AssetReference<DummyAsset> ref(path);

    ASSERT_FALSE(ref.empty());
    ASSERT_TRUE(ref.hasAssetPath());
    ASSERT_FALSE(ref.hasResolvedID());
    ASSERT_NO_THROW(ref.getAssetPath());
    ASSERT_THROW(ref.getID(), std::runtime_error);
}

TEST_F(AssetReferenceTest, FromAssetID)
{
    DummyID id = manager.create(DummyData{});
    trc::AssetReference<DummyAsset> ref(id);

    ASSERT_FALSE(ref.empty());
    ASSERT_FALSE(ref.hasAssetPath());
    ASSERT_TRUE(ref.hasResolvedID());
    ASSERT_THROW(ref.getAssetPath(), std::runtime_error);
    ASSERT_NO_THROW(ref.getID());
}

TEST_F(AssetReferenceTest, ResolveReference)
{
    const trc::AssetPath path("/my/asset/file.txt");
    manager.getDataStorage().store(path, DummyData{});

    trc::AssetReference<DummyAsset> ref(path);
    ASSERT_NO_THROW(ref.resolve(manager));

    ASSERT_FALSE(ref.empty());
    ASSERT_TRUE(ref.hasAssetPath());
    ASSERT_TRUE(ref.hasResolvedID());
    ASSERT_NO_THROW(ref.getAssetPath());
    ASSERT_NO_THROW(ref.getID());
    ASSERT_EQ(manager.getAs<DummyAsset>(path), ref.getID());
}
