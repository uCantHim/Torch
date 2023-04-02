#include <gtest/gtest.h>

#include <string>

#include <trc/assets/AssetTraits.h>
using namespace trc::basic_types;

#include "dummy_asset_type.h"

class AssetTraitStorageTest : public testing::Test
{
protected:
    trc::TraitStorage traits;
};

class Foo : public trc::AssetTrait
{
public:
    virtual auto getNumber() -> ui32 = 0;
};

class Bar : public trc::AssetTrait
{
public:
    virtual auto getString() -> std::string = 0;
};

struct DummyFoo : public Foo { auto getNumber() -> ui32 { return 42; } };
struct DummyBar : public Bar { auto getString() -> std::string { return "dummy"; } };
struct GeoFoo : public Foo { auto getNumber() -> ui32 { return 73; } };
struct GeoBar : public Bar { auto getString() -> std::string { return "geometry"; } };

const trc::AssetType dummyType = trc::AssetType::make<DummyAsset>();
const trc::AssetType geoType = trc::AssetType::make<trc::Geometry>();

TEST_F(AssetTraitStorageTest, UnregisteredReturnNullopt)
{
    ASSERT_FALSE(traits.getTrait<Foo>(dummyType));
    ASSERT_FALSE(traits.getTrait<Bar>(dummyType));
    ASSERT_FALSE(traits.getTrait<Foo>(geoType));
    ASSERT_FALSE(traits.getTrait<Bar>(geoType));
}

TEST_F(AssetTraitStorageTest, StoresCorrectTrait)
{
    traits.registerTrait<Foo>(dummyType, std::make_unique<DummyFoo>());
    traits.registerTrait<Foo>(geoType, std::make_unique<GeoFoo>());

    ASSERT_TRUE(traits.getTrait<Foo>(dummyType));
    ASSERT_TRUE(traits.getTrait<Foo>(geoType));
    ASSERT_NE(nullptr, dynamic_cast<DummyFoo*>(&traits.getTrait<Foo>(dummyType)->get()));
    ASSERT_NE(nullptr, dynamic_cast<GeoFoo*>(&traits.getTrait<Foo>(geoType)->get()));

    Foo& dummyFoo = *traits.getTrait<Foo>(dummyType);
    Foo& geoFoo = *traits.getTrait<Foo>(geoType);
    ASSERT_EQ(dummyFoo.getNumber(), DummyFoo{}.getNumber());
    ASSERT_EQ(geoFoo.getNumber(), GeoFoo{}.getNumber());
}

TEST_F(AssetTraitStorageTest, OverwriteTraits)
{
    ASSERT_NO_THROW(traits.registerTrait<Foo>(dummyType, std::make_unique<DummyFoo>()));
    ASSERT_NO_THROW(traits.registerTrait<Foo>(dummyType, std::make_unique<DummyFoo>()));
    ASSERT_NO_THROW(traits.registerTrait<Foo>(dummyType, std::make_unique<GeoFoo>()));
}
