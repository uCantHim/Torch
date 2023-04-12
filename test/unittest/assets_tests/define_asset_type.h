#pragma once

#include <iostream>
#include <string>
#include <string_view>

#include <trc_util/data/SafeVector.h>

#include <trc/assets/AssetReference.h>
#include <trc/assets/AssetRegistryModuleStorage.h>
#include <trc/assets/AssetSource.h>

using namespace trc::basic_types;

template<typename T>
class TestAssetRegistry : public trc::AssetRegistryModuleInterface<T>
{
public:
    using LocalID = typename trc::AssetRegistryModuleInterface<T>::LocalID;
    using Handle = typename trc::AssetRegistryModuleInterface<T>::Handle;

    void update(vk::CommandBuffer, trc::FrameRenderState&) override {
        ++updateCounter;
    }

    auto add(u_ptr<trc::AssetSource<T>> source) -> LocalID override {
        const LocalID id(nextId++);
        data.emplace(id, source->load());
        return id;
    }

    void remove(LocalID id) override {
        data.erase(id);
    }

    auto getHandle(LocalID id) -> Handle override {
        return Handle{ data.at(id) };
    }

    // Debug/test stuff:

    auto getUpdateCounter() const -> ui32 {
        return updateCounter;
    }

    bool contains(LocalID id) {
        return data.contains(id);
    }

    auto getData(LocalID id) -> trc::AssetData<T>& {
        return data.at(id);
    }

private:
    ui32 updateCounter{ 0 };
    ui32 nextId{ 0 };
    trc::util::SafeVector<trc::AssetData<T>> data;
};

#define DEFINE_ASSET_TYPE(ASSET_NAME, REGISTRY_NAME)                \
                                                                    \
    class REGISTRY_NAME;                                            \
                                                                    \
    struct ASSET_NAME                                               \
    {                                                               \
        using Registry = REGISTRY_NAME;                             \
                                                                    \
        static consteval auto name() -> std::string_view {          \
            return #ASSET_NAME;                                     \
        }                                                           \
    };                                                              \
                                                                    \
    template<>                                                      \
    struct trc::AssetData<ASSET_NAME>                               \
    {                                                               \
        int number{ 0 };                                            \
        std::string string;                                         \
                                                                    \
        void serialize(std::ostream& os) const {                    \
            os << number << string;                                 \
        }                                                           \
                                                                    \
        void deserialize(std::istream& is) {                        \
            is >> number;                                           \
            std::getline(is, string, '\0');                         \
        }                                                           \
    };                                                              \
                                                                    \
    template<>                                                      \
    struct trc::AssetHandle<ASSET_NAME>                             \
    {                                                               \
        auto getNumber() -> int {                                   \
            return data->number;                                    \
        }                                                           \
                                                                    \
        auto getString() -> const std::string& {                    \
            return data->string;                                    \
        }                                                           \
                                                                    \
    private:                                                        \
        friend class TestAssetRegistry<ASSET_NAME>;                 \
        AssetHandle(trc::AssetData<ASSET_NAME>& data)               \
            : data(&data) {}                                        \
                                                                    \
        trc::AssetData<ASSET_NAME>* data;                           \
    };                                                              \
                                                                    \
    class REGISTRY_NAME : public TestAssetRegistry<ASSET_NAME> {};
