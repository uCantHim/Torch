#include <gtest/gtest.h>

#include <trc/assets/AssetStorage.h>
#include <trc/assets/Assets.h>
#include <trc/text/Font.h>
#include <trc/util/FilesystemDataStorage.h>

#include "test_utils.h"

using namespace trc::basic_types;

class AssetStorageTest : public testing::Test
{
protected:
    AssetStorageTest() = default;

    static inline fs::path rootDir{ makeTempDir() };
    trc::AssetStorage storage{ std::make_shared<trc::FilesystemDataStorage>(rootDir) };
};

TEST_F(AssetStorageTest, NonexistingData)
{
    trc::AssetPath paths[]{
        trc::AssetPath("/does_not_exist"),
        trc::AssetPath("does_not_exist_either"),
        trc::AssetPath("/dir/with/empty/data/does_not_exist"),
    };

    for (const trc::AssetPath& path : paths)
    {
        ASSERT_FALSE(storage.getMetadata(path));
        ASSERT_FALSE(storage.load<trc::Geometry>(path));
        ASSERT_FALSE(storage.load<trc::Material>(path));
        ASSERT_FALSE(storage.load<trc::Rig>(path));
        ASSERT_FALSE(storage.remove(path));
    }
}

TEST_F(AssetStorageTest, BasicStore)
{
    trc::AssetPath path("/foo/myasset");

    trc::GeometryData data;
    data.indices = { 1, 2, 5, 4, 6, 3 };
    data.vertices = { trc::MeshVertex{ vec3(0.0f), vec3(0.0f), vec2(0.2f, 0.6f), vec3(0.0f) } };

    storage.store(path, data);

    auto loadedData = storage.load<trc::Geometry>(path);
    ASSERT_EQ(data.indices, loadedData->indices);
    ASSERT_EQ(data.vertices.size(), loadedData->vertices.size());
    ASSERT_FALSE(loadedData->rig.hasAssetPath());
    ASSERT_TRUE(loadedData->rig.empty());

    ASSERT_FALSE(storage.load<trc::Texture>(path));
    ASSERT_FALSE(storage.load<trc::Font>(path));
}

TEST_F(AssetStorageTest, MetadataStore)
{
    trc::AssetPath dataPath("/thing");
    storage.store(dataPath, trc::makeCubeGeo());

    auto meta = storage.getMetadata(dataPath);
    ASSERT_TRUE(meta.has_value());

    ASSERT_EQ(meta->name, "thing");
    ASSERT_TRUE(meta->path.has_value());
    ASSERT_EQ(meta->path.value(), dataPath);

    ASSERT_TRUE(meta->type.is<trc::Geometry>());
    ASSERT_FALSE(meta->type.is<trc::Font>());
    ASSERT_FALSE(meta->type.is<trc::Rig>());
    ASSERT_FALSE(meta->type.is<trc::Animation>());
}

TEST_F(AssetStorageTest, Remove)
{
    trc::AssetPath path("/bar/baz/removed.data");
    ASSERT_TRUE(storage.store(path, trc::RigData{}));
    ASSERT_TRUE(storage.remove(path));
    ASSERT_FALSE(storage.load<trc::Rig>(path));
    ASSERT_FALSE(storage.remove(path));

    ASSERT_TRUE(storage.store(path, trc::AssetData<trc::Font>{}));
    ASSERT_TRUE(storage.remove(path));
    ASSERT_FALSE(storage.load<trc::Font>(path));
    ASSERT_FALSE(storage.remove(path));
}

TEST_F(AssetStorageTest, OverwriteData)
{
    trc::AssetPath path("/data_to_overwrite");
    ASSERT_TRUE(storage.store(path, trc::GeometryData{}));
    ASSERT_TRUE(storage.getMetadata(path).has_value());
    ASSERT_TRUE(storage.getMetadata(path)->type.is<trc::Geometry>());

    ASSERT_TRUE(storage.store(path, trc::AnimationData{}));
    ASSERT_TRUE(storage.getMetadata(path).has_value());
    ASSERT_TRUE(storage.getMetadata(path)->type.is<trc::Animation>());

    ASSERT_TRUE(storage.store(path, trc::RigData{}));
    ASSERT_TRUE(storage.getMetadata(path).has_value());
    ASSERT_TRUE(storage.getMetadata(path)->type.is<trc::Rig>());
}
