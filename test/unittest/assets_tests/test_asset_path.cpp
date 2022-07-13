#include <gtest/gtest.h>

#include <trc/assets/AssetPath.h>
#include <trc/util/TorchDirectories.h>

TEST(AssetPathTest, InvalidPathThrows)
{
    ASSERT_NO_THROW(trc::AssetPath("./nested/file"));
    ASSERT_NO_THROW(trc::AssetPath("many/nested/../directories/file"));
    ASSERT_NO_THROW(trc::AssetPath("/foo/bar"));

    ASSERT_THROW(trc::AssetPath(""), std::invalid_argument);
    ASSERT_THROW(trc::AssetPath("."), std::invalid_argument);
    ASSERT_THROW(trc::AssetPath("/"), std::invalid_argument);
    ASSERT_THROW(trc::AssetPath("subdir/"), std::invalid_argument);
    ASSERT_THROW(trc::AssetPath("test/../../other/dir"), std::invalid_argument);
    // Might be accidentally correct, but it is not allowed to start with ..
    ASSERT_THROW(trc::AssetPath("../assets/foo"), std::invalid_argument);
    ASSERT_THROW(trc::AssetPath("/../rootpath"), std::invalid_argument);
    ASSERT_THROW(trc::AssetPath("/foo/bar/../../.."), std::invalid_argument);
}

TEST(AssetPathTest, AssetName)
{
    ASSERT_EQ(trc::AssetPath("foo").getAssetName(), "foo");
    ASSERT_EQ(trc::AssetPath("foo.txt").getAssetName(), "foo");
    ASSERT_EQ(trc::AssetPath("foo/bar/baz.md").getAssetName(), "baz");
    ASSERT_EQ(trc::AssetPath("/thing.awdekse").getAssetName(), "thing");
}
