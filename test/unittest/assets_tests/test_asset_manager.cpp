#include <gtest/gtest.h>

#include <trc/assets/AssetManager.h>

#include "assets_tests/memory_data_storage.h"
#include "define_asset_type.h"

DEFINE_ASSET_TYPE(Foo, FooRegistry);
using FooID = trc::TypedAssetID<Foo>;
using FooData = trc::AssetData<Foo>;

DEFINE_ASSET_TYPE(Bar, BarRegistry);
using BarID = trc::TypedAssetID<Bar>;
using BarData = trc::AssetData<Bar>;

DEFINE_ASSET_TYPE(Baz, BazRegistry);
using BazID = trc::TypedAssetID<Baz>;
using BazData = trc::AssetData<Baz>;


//////////////////////////////////////////////////////
///     Declare an asset type with references      ///
//////////////////////////////////////////////////////

class ReferencingAssetRegistry;

struct ReferencingAsset
{
    using Registry = ReferencingAssetRegistry;
    static auto name() -> std::string_view {
        return "referencing_asset";
    }
};

template<>
struct trc::AssetData<ReferencingAsset>
{
    int number{ 0 };
    AssetReference<Foo> fooRef;

    void serialize(std::ostream& os) const {
        os << number << fooRef.getAssetPath().string();
    }

    void deserialize(std::istream& is)
    {
        is >> number;
        std::string str;
        std::getline(is, str, '\0');
        fooRef = trc::AssetPath(str);
    }

    void resolveReferences(AssetManager& assets) {
        fooRef.resolve(assets);
    }
};

template<>
struct trc::AssetHandle<ReferencingAsset>
{
    AssetHandle(AssetData<ReferencingAsset>& data) : data(&data) {}
    AssetData<ReferencingAsset>* data;
};

class ReferencingAssetRegistry : public TestAssetRegistry<ReferencingAsset> {};


///////////////////////////////////////
///     Declare the test class      ///
///////////////////////////////////////

class AssetManagerTest : public testing::Test
{
protected:
    AssetManagerTest()
        :
        assets(std::make_shared<MemoryStorage>()),
        storage(assets.getDataStorage())
    {
        assets.registerAssetType<Foo>(std::make_unique<FooRegistry>());
        assets.registerAssetType<Bar>(std::make_unique<BarRegistry>());
    }

    trc::AssetManager assets;
    trc::AssetStorage& storage;
};

inline auto path(std::string_view str) -> trc::AssetPath {
    return trc::AssetPath(str);
}


//////////////////////////////////
///     Declare the tests      ///
//////////////////////////////////

TEST_F(AssetManagerTest, CreateFromData)
{
    const FooID foo = assets.create(FooData{ .number=0, .string="I am a Foo!" });
    const BarID bar = assets.create(BarData{ .number=42, .string="This Bar contains the answer." });
    const FooID foo2 = assets.create(FooData{ .number=-123, .string="I am a Foo!" });
    const BarID bar2 = assets.create(BarData{ .number=-321, .string="This Bar does not have the answer." });

    ASSERT_NE(foo, foo2);
    ASSERT_NE(bar, bar2);

    ASSERT_EQ(assets.getHandle(foo).getNumber(), 0);
    ASSERT_EQ(assets.getHandle(bar).getNumber(), 42);
    ASSERT_EQ(assets.getHandle(foo2).getNumber(), -123);
    ASSERT_EQ(assets.getHandle(bar2).getNumber(), -321);
    ASSERT_EQ(assets.getHandle(foo).getString(), "I am a Foo!");
    ASSERT_EQ(assets.getHandle(bar).getString(), "This Bar contains the answer.");
    ASSERT_EQ(assets.getHandle(foo2).getString(), "I am a Foo!");
    ASSERT_EQ(assets.getHandle(bar2).getString(), "This Bar does not have the answer.");
}

TEST_F(AssetManagerTest, CreateFromSource)
{
    struct MySource : public trc::AssetSource<Foo>
    {
    public:
        auto load() -> FooData override {
            return FooData{ .number=999, .string="generated" };
        }

        auto getMetadata() -> trc::AssetMetadata override {
            return { .name="generated_foo", .type=trc::AssetType::make<Foo>() };
        }
    };

    const trc::AssetPath path1("/asset_source_test_data");
    const trc::AssetPath path2("/other/bar");
    storage.store(path1, BarData{ .number=50, .string="Hello World!" });
    storage.store(path2, BarData{ .number=60, .string="Hello Sky!" });
    auto source1 = storage.loadDeferred<Bar>(path1);
    auto source2 = storage.loadDeferred<Bar>(path2);
    ASSERT_TRUE(source1);
    ASSERT_TRUE(source2);

    ASSERT_THROW(assets.create<Foo>(nullptr), std::invalid_argument);
    ASSERT_THROW(assets.create<Bar>(nullptr), std::invalid_argument);
    ASSERT_THROW(assets.create<Baz>(nullptr), std::invalid_argument);
    FooID foo = assets.create<Foo>(std::make_unique<MySource>());
    BarID bar1 = assets.create(std::move(*source1));
    BarID bar2 = assets.create(std::move(*source2));

    ASSERT_EQ(foo.getMetadata().name, "generated_foo");
    ASSERT_EQ(bar1.getMetadata().name, "asset_source_test_data");
    ASSERT_EQ(bar2.getMetadata().name, "bar");
    ASSERT_EQ(foo.getMetadata().type, trc::AssetType::make<Foo>());
    ASSERT_EQ(bar1.getMetadata().type, trc::AssetType::make<Bar>());
    ASSERT_EQ(bar2.getMetadata().type, trc::AssetType::make<Bar>());
    ASSERT_EQ(foo.getMetadata().path, std::nullopt);
    ASSERT_EQ(bar1.getMetadata().path, path1);
    ASSERT_EQ(bar2.getMetadata().path, path2);

    ASSERT_EQ(assets.getHandle(foo).getNumber(), 999);
    ASSERT_EQ(assets.getHandle(bar1).getNumber(), 50);
    ASSERT_EQ(assets.getHandle(bar2).getNumber(), 60);
    ASSERT_EQ(assets.getHandle(foo).getString(), "generated");
    ASSERT_EQ(assets.getHandle(bar1).getString(), "Hello World!");
    ASSERT_EQ(assets.getHandle(bar2).getString(), "Hello Sky!");
}

TEST_F(AssetManagerTest, InvalidPath)
{
    ASSERT_FALSE(assets.create(trc::AssetPath("/does/not/exist")));
    ASSERT_FALSE(assets.create<Bar>(trc::AssetPath("invalid_asset")));
}

TEST_F(AssetManagerTest, CreateFromPathExplicitType)
{
    const trc::AssetPath pathFoo("my/foo");
    const trc::AssetPath pathBar("my/bar");
    storage.store(pathFoo, FooData{ .number=-9876, .string="foo" });
    storage.store(pathBar, BarData{ .number=1234, .string="bar" });

    // Explicitly typed `create`
    auto fooTyped = assets.create<Foo>(pathFoo);
    auto barTyped = assets.create<Bar>(pathBar);
    ASSERT_TRUE(fooTyped);
    ASSERT_TRUE(barTyped);

    // Duplicate `create` for the same path returns the existing asset
    ASSERT_EQ(fooTyped, assets.create<Foo>(pathFoo));
    ASSERT_EQ(barTyped, assets.create<Bar>(pathBar));

    // Wrong explicit type throws
    ASSERT_THROW(assets.create<Bar>(pathFoo), std::invalid_argument);
    ASSERT_THROW(assets.create<Foo>(pathBar), std::invalid_argument);
}

TEST_F(AssetManagerTest, CreateFromPathImplicitType)
{
    const trc::AssetPath pathFoo("my/foo");
    const trc::AssetPath pathBar("my/bar");
    storage.store(pathFoo, FooData{ .number=-9876, .string="foo" });
    storage.store(pathBar, BarData{ .number=1234, .string="bar" });

    auto foo = assets.create(pathFoo);
    auto bar = assets.create(pathBar);
    ASSERT_TRUE(foo);
    ASSERT_TRUE(bar);
    ASSERT_TRUE(assets.getAs<Foo>(*foo));
    ASSERT_TRUE(assets.getAs<Bar>(*bar));

    // Casting to wrong type fails
    ASSERT_FALSE(assets.getAs<Foo>(*bar));
    ASSERT_FALSE(assets.getAs<Bar>(*foo));

    // Duplicate `create` for the same path returns the existing asset
    ASSERT_EQ(foo, assets.create(pathFoo));
    ASSERT_EQ(bar, assets.create(pathBar));

    auto fooHandle = assets.getHandle(*assets.getAs<Foo>(*foo));
    ASSERT_EQ(fooHandle.getNumber(), -9876);
    ASSERT_EQ(fooHandle.getString(), "foo");
    auto barHandle = assets.getHandle(*assets.getAs<Bar>(*bar));
    ASSERT_EQ(barHandle.getNumber(), 1234);
    ASSERT_EQ(barHandle.getString(), "bar");
}

TEST_F(AssetManagerTest, CreateUnregisteredAssetTypeFails)
{
    const trc::AssetPath path("/path/to/unregistered/asset");
    storage.store(path, BazData{});

    auto source = std::make_unique<trc::InMemorySource<Baz>>(BazData{});
    ASSERT_THROW(assets.create<Baz>(std::move(source)), std::out_of_range);
    ASSERT_THROW(assets.create(BazData{}), std::out_of_range);
    ASSERT_THROW(assets.create(path), std::out_of_range);

    ASSERT_NO_THROW(assets.create(FooData{}));
    ASSERT_NO_THROW(assets.create(trc::AssetPath("/some/other/path")));

    assets.registerAssetType<Baz>(std::make_unique<BazRegistry>());
    ASSERT_NO_THROW(assets.create<Baz>(std::make_unique<trc::InMemorySource<Baz>>(BazData{})));
    ASSERT_NO_THROW(assets.create(BazData{}));
    ASSERT_NO_THROW(assets.create(path));
}

TEST_F(AssetManagerTest, DestroyFromIdRemovesPath)
{
    const trc::AssetPath path("/bar/first_asset");
    storage.store(path, BarData{});

    const trc::AssetID id = assets.create(path).value();
    assets.destroy(id);

    ASSERT_FALSE(assets.exists(path));
    ASSERT_THROW(assets.getAs<Bar>(id), trc::InvalidAssetIdError);
    ASSERT_FALSE(assets.getAs<Bar>(path));
    ASSERT_THROW(assets.destroy<Bar>(id), trc::InvalidAssetIdError);
    ASSERT_NO_THROW(assets.destroy(path));
}

TEST_F(AssetManagerTest, ResolveReferences)
{
    assets.registerAssetType<ReferencingAsset>(std::make_unique<ReferencingAssetRegistry>());

    const trc::AssetPath fooPath("/my_foo");
    const trc::AssetPath dummyPath("/dir/dummy");
    storage.store(fooPath, FooData{ .number=42, .string="hi" });
    storage.store(dummyPath, trc::AssetData<ReferencingAsset>{ .number=-2, .fooRef=fooPath });

    ASSERT_FALSE(assets.getAs<Foo>(fooPath));

    // Creating the referencing asset also creates the referenced asset
    auto dummy = assets.create<ReferencingAsset>(dummyPath);
    ASSERT_TRUE(dummy);
    ASSERT_TRUE(assets.getAs<Foo>(fooPath));
    ASSERT_TRUE(assets.getAs<ReferencingAsset>(dummyPath));

    // The reference is now resolved and contains the ID of the created Foo asset
    auto handle = assets.getHandle(*dummy);
    ASSERT_TRUE(handle.data->fooRef.hasResolvedID());
    ASSERT_EQ(assets.getAs<Foo>(fooPath), handle.data->fooRef.getID());

    // Destroying the referencing asset leaves the referenced asset intact
    assets.destroy(dummyPath);
    ASSERT_FALSE(assets.exists(dummyPath));
    ASSERT_TRUE(assets.getAs<Foo>(fooPath));
}
