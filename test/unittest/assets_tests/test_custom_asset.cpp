#include <gtest/gtest.h>

#include <array>
#include <fstream>
#include <sstream>

#include <trc/assets/AssetBase.h>
#include <trc/assets/AssetRegistryModule.h>
#include <trc/assets/AssetManager.h>
#include <trc/core/Instance.h>
#include <trc/util/TorchDirectories.h>
#include <trc/Torch.h>

using namespace trc::basic_types;

class HitboxRegistry;

struct Hitbox
{
    using Registry = HitboxRegistry;
};

template<>
struct trc::AssetData<Hitbox>
{
    vec3 offset;
    float radius;

    void serialize(std::ostream& os) const {
        os.write(reinterpret_cast<const char*>(this), sizeof(AssetData<Hitbox>));
    }
    void deserialize(std::istream& is) {
        is.read(reinterpret_cast<char*>(this), sizeof(AssetData<Hitbox>));
    }
};

template<>
struct trc::AssetHandle<Hitbox>
{
    AssetData<Hitbox> data;
};

using HitboxData = trc::AssetData<Hitbox>;
using HitboxHandle = trc::AssetHandle<Hitbox>;

class HitboxRegistry : public trc::AssetRegistryModuleInterface<Hitbox>
{
public:
    HitboxRegistry() = default;

    void update(vk::CommandBuffer, trc::FrameRenderState&) override {}

    auto add(u_ptr<trc::AssetSource<Hitbox>> source) -> LocalID override
    {
        hitboxes.emplace_back(source->load());
        return LocalID(hitboxes.size() - 1);
    }

    void remove(LocalID id) override {}

    auto getHandle(LocalID id) -> trc::AssetHandle<Hitbox> override {
        return { hitboxes.at(id) };
    }

    auto getHitboxes() -> std::vector<HitboxData> {
        return hitboxes;
    }

private:
    std::vector<HitboxData> hitboxes;
};

/**
 * The test
 */
class CustomAssetTest : public testing::Test
{
public:
    CustomAssetTest()
        :
        instance([]{
            trc::init();
            return trc::Instance(trc::InstanceCreateInfo{ .enableRayTracing=false });
        }()),
        assets(instance, { .enableRayTracing=false }),
        deviceRegistry(assets.getDeviceRegistry())
    {
        trc::util::setProjectDirectory(TMPDIR);
        fs::create_directories(trc::util::getAssetStorageDirectory());
    }

    ~CustomAssetTest() {
        trc::terminate();
    }

    trc::Instance instance;
    trc::AssetManager assets;
    trc::AssetRegistry& deviceRegistry;
};

TEST_F(CustomAssetTest, CreateAndDestroy)
{
    assets.getDeviceRegistry().addModule<Hitbox>();
    auto& module = assets.getModule<Hitbox>();

    std::vector<trc::TypedAssetID<Hitbox>> ids;
    ASSERT_NO_THROW(ids.emplace_back(assets.create(HitboxData{ vec3(0.0f), 4 })));
    ASSERT_NO_THROW(ids.emplace_back(assets.create(HitboxData{ vec3(5.432f), 77 })));
    ASSERT_NO_THROW(assets.getMetaData(ids[0]));
    ASSERT_NO_THROW(assets.getMetaData(ids[1]));

    auto hitboxes = module.getHitboxes();
    ASSERT_EQ(hitboxes.size(), 2);
    ASSERT_EQ(hitboxes[0].offset, vec3(0.0f));
    ASSERT_EQ(hitboxes[0].radius, 4);
    ASSERT_EQ(hitboxes[1].radius, 77);

    ASSERT_NO_THROW(assets.destroy(ids[0]));
}

TEST_F(CustomAssetTest, UnregisteredModuleThrows)
{
    auto source = std::make_unique<trc::InMemorySource<Hitbox>>(HitboxData{});
    ASSERT_THROW(deviceRegistry.add<Hitbox>(std::move(source)), std::out_of_range);
    ASSERT_THROW(assets.create(HitboxData{}), std::out_of_range);
}

TEST_F(CustomAssetTest, CreateViaAssetPath)
{
    assets.getDeviceRegistry().addModule<Hitbox>();

    const trc::AssetPath path("hitbox_custom_asset.ta");
    {
        std::ofstream file(path.getFilesystemPath(), std::ios::binary);
        HitboxData hitboxData{ .offset=vec3(1, 2.5, 4.5), .radius=1234.56 };
        hitboxData.serialize(file);
    }
    ASSERT_TRUE(fs::is_regular_file(path.getFilesystemPath()));

    auto id = assets.create<Hitbox>(path);
    ASSERT_TRUE(assets.exists(path));

    auto hitboxes = assets.getModule<Hitbox>().getHitboxes();
    ASSERT_EQ(hitboxes.size(), 1);
    ASSERT_FLOAT_EQ(hitboxes[0].offset.x, 1.0f);
    ASSERT_FLOAT_EQ(hitboxes[0].offset.y, 2.5f);
    ASSERT_FLOAT_EQ(hitboxes[0].offset.z, 4.5);
    ASSERT_FLOAT_EQ(hitboxes[0].radius, 1234.56f);

    ASSERT_NO_THROW(assets.destroy(path));
    ASSERT_FALSE(assets.exists(path));
}
