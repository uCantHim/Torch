#include <sstream>

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include "AssetFileIndex.h"

using nlohmann::json;

TEST(AssetFileIndexTest, InsertErase)
{
    AssetFileIndex index;

    index.insert<trc::Rig>(trc::AssetPath("foo"));
    index.insert<trc::Rig>(trc::AssetPath("bar"));
    index.insert<trc::Rig>(trc::AssetPath("baz"));
    ASSERT_TRUE(index.contains(trc::AssetPath("foo")));
    ASSERT_TRUE(index.contains(trc::AssetPath("bar")));
    ASSERT_TRUE(index.contains(trc::AssetPath("baz")));

    index.erase(trc::AssetPath("bar"));
    ASSERT_TRUE(index.contains(trc::AssetPath("foo")));
    ASSERT_FALSE(index.contains(trc::AssetPath("bar")));
    ASSERT_TRUE(index.contains(trc::AssetPath("baz")));

    index.erase(trc::AssetPath("foo"));
    index.erase(trc::AssetPath("baz"));
    ASSERT_FALSE(index.contains(trc::AssetPath("foo")));
    ASSERT_FALSE(index.contains(trc::AssetPath("bar")));
    ASSERT_FALSE(index.contains(trc::AssetPath("baz")));
}

TEST(AssetFileIndexTest, SaveLoad)
{
    std::stringstream ss;

    {
        AssetFileIndex index;
        index.insert<trc::Rig>(trc::AssetPath("foo"));
        index.insert<trc::Animation>(trc::AssetPath("bar"));
        index.insert<trc::Texture>(trc::AssetPath("baz"));
        index.save(ss);
    }

    AssetFileIndex index;
    index.load(ss);

    index.erase(trc::AssetPath("bar"));
    ASSERT_TRUE(index.contains(trc::AssetPath("foo")));
    ASSERT_FALSE(index.contains(trc::AssetPath("bar")));
    ASSERT_TRUE(index.contains(trc::AssetPath("baz")));
    ASSERT_EQ(index.getType(trc::AssetPath("foo")), AssetType::eRig);
    ASSERT_EQ(index.getType(trc::AssetPath("bar")), std::nullopt);
    ASSERT_EQ(index.getType(trc::AssetPath("baz")), AssetType::eTexture);

    index.erase(trc::AssetPath("foo"));
    index.erase(trc::AssetPath("baz"));
    index.save(ss);
}

TEST(AssetFileIndexTest, LoadInvalidJsonThrows)
{
    AssetFileIndex index;

    std::stringstream ss;
    ASSERT_THROW(index.load(ss), std::runtime_error);
}
