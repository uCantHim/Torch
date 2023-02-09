#include <gtest/gtest.h>

#include <trc/assets/AssetType.h>
#include <trc/assets/Assets.h>

using trc::AssetType;

struct MyTestAssetRegistry;

struct MyTestAsset
{
    using Registry = MyTestAssetRegistry;
    static consteval auto name() -> std::string_view {
        return "my_test_asset";
    }
};

template<>
struct trc::AssetData<MyTestAsset>
{
    void serialize(std::ostream&) const;
    void deserialize(std::istream&);
};

struct MyTestAssetRegistry : public trc::AssetRegistryModuleInterface<MyTestAsset> {};

TEST(AssetTypeTest, ConstructionFromType)
{
    ASSERT_TRUE(AssetType::make<trc::Geometry>().is<trc::Geometry>());
    ASSERT_TRUE(AssetType::make<trc::Animation>().is<trc::Animation>());
    ASSERT_TRUE(AssetType::make<trc::Font>().is<trc::Font>());
    ASSERT_TRUE(AssetType::make<trc::Material>().is<trc::Material>());
    ASSERT_TRUE(AssetType::make<trc::Rig>().is<trc::Rig>());
    ASSERT_TRUE(AssetType::make<trc::Texture>().is<trc::Texture>());

    ASSERT_EQ(AssetType::make<trc::Geometry>().getName(), trc::Geometry::name());
    ASSERT_EQ(AssetType::make<trc::Animation>().getName(), trc::Animation::name());
    ASSERT_EQ(AssetType::make<trc::Font>().getName(), trc::Font::name());
    ASSERT_EQ(AssetType::make<trc::Material>().getName(), trc::Material::name());
    ASSERT_EQ(AssetType::make<trc::Rig>().getName(), trc::Rig::name());
    ASSERT_EQ(AssetType::make<trc::Texture>().getName(), trc::Texture::name());
}

TEST(AssetTypeTest, ConstructionFromString)
{
    ASSERT_EQ(AssetType::make<trc::Geometry>(),  AssetType::make(std::string{trc::Geometry::name()}));
    ASSERT_EQ(AssetType::make<trc::Animation>(), AssetType::make(std::string{trc::Animation::name()}));
    ASSERT_EQ(AssetType::make<trc::Font>(),      AssetType::make(std::string{trc::Font::name()}));
    ASSERT_EQ(AssetType::make<trc::Material>(),  AssetType::make(std::string{trc::Material::name()}));
    ASSERT_EQ(AssetType::make<trc::Rig>(),       AssetType::make(std::string{trc::Rig::name()}));
    ASSERT_EQ(AssetType::make<trc::Texture>(),   AssetType::make(std::string{trc::Texture::name()}));

    ASSERT_FALSE(AssetType::make("_foo_").is<trc::Material>());
    ASSERT_FALSE(AssetType::make("foobar").is<trc::Material>());
    ASSERT_FALSE(AssetType::make("other thing").is<trc::Material>());
    ASSERT_FALSE(AssetType::make("Hello World!").is<trc::Material>());
}

TEST(AssetTypeTest, CustomAssetType)
{
    const AssetType type = AssetType::make<MyTestAsset>();
    ASSERT_TRUE(type.is<MyTestAsset>());
    ASSERT_EQ(type.getName(), "my_test_asset");
    ASSERT_EQ(type, AssetType::make("my_test_asset"));
    ASSERT_NE(type, AssetType::make("foobar"));
}
