#include <gtest/gtest.h>

#include <trc/assets/AssetManager.h>
#include <trc/assets/AssetReference.h>
#include <trc/util/FilesystemDataStorage.h>
#include <trc_util/MemoryStream.h>

#include "dummy_asset_type.h"
#include "test_utils.h"

/**
 * A simple in-memory implementation of trc::DataStorage. We use this to store
 * data at asset paths.
 */
class MemoryStorage : public trc::DataStorage
{
public:
    auto read(const path& path) -> s_ptr<std::istream> override {
        auto it = storage.find(path);
        if (it == storage.end()) {
            return nullptr;
        }

        auto& data = it->second;
        return std::make_shared<trc::util::MemoryStream>((char*)data.data(), data.size());
    }

    auto write(const path& path) -> s_ptr<std::ostream> override {
        auto [it, success] = storage.try_emplace(path, std::vector<std::byte>(1000000));
        auto& data = it->second;
        return std::make_shared<trc::util::MemoryStream>((char*)data.data(), data.size());
    }

    bool remove(const path& path) override {
        return storage.erase(path) > 0;
    }
private:
    std::unordered_map<path, std::vector<std::byte>> storage;
};

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
