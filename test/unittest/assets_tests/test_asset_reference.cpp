#include <gtest/gtest.h>

#include <trc/assets/AssetReference.h>
#include <trc/assets/Geometry.h>
#include <trc/core/Instance.h>
#include <trc/Torch.h>

using T = trc::Geometry;

TEST(AssetReferenceTest, EmptyReference)
{
    trc::AssetReference<T> ref;

    ASSERT_TRUE(ref.empty());
    ASSERT_FALSE(ref.hasAssetPath());
    ASSERT_FALSE(ref.hasResolvedID());
    ASSERT_THROW(ref.getAssetPath(), std::runtime_error);
    ASSERT_THROW(ref.getID(), std::runtime_error);
}

TEST(AssetReferenceTest, FromAssetPath)
{
    trc::AssetPath path("my/asset/file.txt");
    trc::AssetReference<T> ref(path);

    ASSERT_FALSE(ref.empty());
    ASSERT_TRUE(ref.hasAssetPath());
    ASSERT_FALSE(ref.hasResolvedID());
    ASSERT_NO_THROW(ref.getAssetPath());
    ASSERT_THROW(ref.getID(), std::runtime_error);
}

TEST(AssetReferenceTest, FromAssetID)
{
    trc::init();
    trc::Instance instance{};
    trc::AssetManager man(instance, {});

    trc::GeometryID id = man.create(trc::GeometryData{});
    trc::AssetReference<T> ref(id);

    ASSERT_FALSE(ref.empty());
    ASSERT_FALSE(ref.hasAssetPath());
    ASSERT_TRUE(ref.hasResolvedID());
    ASSERT_THROW(ref.getAssetPath(), std::runtime_error);
    ASSERT_NO_THROW(ref.getID());

    trc::terminate();
}

TEST(AssetReferenceTest, ResolveReference)
{
    trc::init();
    trc::Instance instance{};
    trc::AssetManager man(instance, {});

    trc::AssetPath path("my/asset/file.txt");
    trc::AssetReference<T> ref(path);
    ref.resolve(man);

    ASSERT_FALSE(ref.empty());
    ASSERT_TRUE(ref.hasAssetPath());
    ASSERT_TRUE(ref.hasResolvedID());
    ASSERT_NO_THROW(ref.getAssetPath());
    ASSERT_NO_THROW(ref.getID());
    ASSERT_EQ(man.get<trc::Geometry>(path), ref.getID());

    trc::terminate();
}
