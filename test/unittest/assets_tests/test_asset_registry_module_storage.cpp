#include <gtest/gtest.h>

#include <trc/assets/AssetRegistryModuleStorage.h>
#include <trc/assets/GeometryRegistry.h>
#include <trc/assets/MaterialRegistry.h>
#include <trc/assets/TextureRegistry.h>
#include <trc/core/Frame.h>

#include "define_asset_type.h"

DEFINE_ASSET_TYPE(DummyAsset, DummyRegistry);
using DummyID = trc::TypedAssetID<DummyAsset>;
using DummyData = trc::AssetData<DummyAsset>;

class AssetRegistryModuleStorageTest : public testing::Test
{
protected:
    trc::AssetRegistryModuleStorage storage;
};

TEST_F(AssetRegistryModuleStorageTest, HasModule)
{
    ASSERT_FALSE(storage.hasModule<trc::Geometry>());
    ASSERT_FALSE(storage.hasModule<trc::Material>());
    ASSERT_FALSE(storage.hasModule<trc::Texture>());
    ASSERT_FALSE(storage.hasModule<DummyAsset>());
    ASSERT_THROW(storage.getModule<trc::Geometry>(), std::out_of_range);
    ASSERT_THROW(storage.getModule<trc::Material>(), std::out_of_range);
    ASSERT_THROW(storage.getModule<trc::Texture>(), std::out_of_range);
    ASSERT_THROW(storage.getModule<DummyAsset>(), std::out_of_range);

    storage.addModule<DummyAsset>(std::make_unique<DummyRegistry>());
    ASSERT_FALSE(storage.hasModule<trc::Geometry>());
    ASSERT_FALSE(storage.hasModule<trc::Material>());
    ASSERT_FALSE(storage.hasModule<trc::Texture>());
    ASSERT_TRUE(storage.hasModule<DummyAsset>());
    ASSERT_THROW(storage.getModule<trc::Geometry>(), std::out_of_range);
    ASSERT_THROW(storage.getModule<trc::Material>(), std::out_of_range);
    ASSERT_THROW(storage.getModule<trc::Texture>(), std::out_of_range);
    ASSERT_NO_THROW(storage.getModule<DummyAsset>());
}

TEST_F(AssetRegistryModuleStorageTest, RegisterModuleTwice)
{
    ASSERT_NO_THROW(storage.addModule<DummyAsset>(std::make_unique<DummyRegistry>()));
    ASSERT_THROW(storage.addModule<DummyAsset>(std::make_unique<DummyRegistry>()), std::out_of_range);
}

TEST_F(AssetRegistryModuleStorageTest, RegisterNullptrArgumentThrows)
{
    ASSERT_THROW(storage.addModule<DummyAsset>(nullptr), std::invalid_argument);
    ASSERT_NO_THROW(storage.addModule<DummyAsset>(std::make_unique<DummyRegistry>()));
    ASSERT_THROW(storage.addModule<DummyAsset>(nullptr), std::invalid_argument);
}

TEST_F(AssetRegistryModuleStorageTest, StoresCorrectObject)
{
    trc::FrameRenderState state;

    auto _mod = std::make_unique<DummyRegistry>();
    auto mod = _mod.get();  // Pointer to stored module

    mod->update({}, state);
    storage.addModule<DummyAsset>(std::move(_mod));
    mod->update({}, state);

    ASSERT_EQ(mod, &storage.getModule<DummyAsset>());
    ASSERT_EQ(mod->getUpdateCounter(), 2);
    ASSERT_EQ(storage.getModule<DummyAsset>().getUpdateCounter(), 2);
}

TEST_F(AssetRegistryModuleStorageTest, Update)
{
    storage.addModule<DummyAsset>(std::make_unique<DummyRegistry>());
    DummyRegistry& mod = storage.getModule<DummyAsset>();

    trc::FrameRenderState state;
    ASSERT_EQ(mod.getUpdateCounter(), 0);
    mod.update({}, state);
    ASSERT_EQ(mod.getUpdateCounter(), 1);

    // Update the module through storage.update():
    for (int i = 0; i < 20; ++i) {
        storage.update({}, state);
    }
    ASSERT_EQ(mod.getUpdateCounter(), 21);
}
