#pragma once

#include <string>
#include <string_view>

#include <trc/assets/Assets.h>
#include <trc/assets/AssetSource.h>

using namespace trc::basic_types;

class DummyRegistry;

struct DummyAsset
{
    using Registry = DummyRegistry;

    static consteval auto name() -> std::string_view {
        return "dummy_asset_type";
    }
};

template<>
struct trc::AssetData<DummyAsset>
{
    int number{ 0 };
    std::string string;

    void serialize(std::ostream& os) const {
        os << number << string;
    }

    void deserialize(std::istream& is) {
        is >> number >> string;
    }
};

template<>
struct trc::AssetHandle<DummyAsset>
{
};

class DummyRegistry : public trc::AssetRegistryModuleInterface<DummyAsset>
{
public:
    void update(vk::CommandBuffer, trc::FrameRenderState&) override {
        ++updateCounter;
    }

    auto add(u_ptr<trc::AssetSource<DummyAsset>> source) -> LocalID override {
        const LocalID id(nextId++);
        data.try_emplace(id, source->load());
        return id;
    }

    void remove(LocalID id) override {
        data.erase(id);
    }

    auto getHandle(LocalID) -> Handle override {
        return {};
    }

    // Debug/test stuff:

    auto getUpdateCounter() const -> ui32 {
        return updateCounter;
    }

    bool contains(LocalID id) {
        return data.contains(id);
    }

    auto getData(LocalID id) -> trc::AssetData<DummyAsset>& {
        return data.at(id);
    }

private:
    ui32 updateCounter{ 0 };
    ui32 nextId{ 0 };
    std::unordered_map<LocalID, trc::AssetData<DummyAsset>> data;
};

using DummyID = trc::TypedAssetID<DummyAsset>;
using DummyData = trc::AssetData<DummyAsset>;
