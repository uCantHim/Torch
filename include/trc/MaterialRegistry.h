#pragma once

#include <vkb/Buffer.h>
#include <trc_util/data/IndexMap.h>
#include <trc_util/data/ObjectId.h>

#include "Types.h"
#include "AssetIds.h"
#include "Material.h"
#include "AssetRegistryModule.h"

namespace trc
{
    class MaterialRegistry : public AssetRegistryModuleInterface
    {
    public:
        using LocalID = MaterialID::LocalID;
        using Handle = MaterialDeviceHandle;

        MaterialRegistry(const AssetRegistryModuleCreateInfo& info);

        void update(vk::CommandBuffer cmdBuf) final;

        auto getDescriptorLayoutBindings() -> std::vector<DescriptorLayoutBindingInfo> final;
        auto getDescriptorUpdates() -> std::vector<vk::WriteDescriptorSet> final;

        auto add(const MaterialDeviceHandle& data) -> LocalID;
        void remove(LocalID id);

        auto getHandle(LocalID id) -> MaterialDeviceHandle;

    private:
        static constexpr ui32 MATERIAL_BUFFER_DEFAULT_SIZE = sizeof(MaterialDeviceHandle) * 100;

        const AssetRegistryModuleCreateInfo config;

        data::IdPool idPool;
        data::IndexMap<LocalID::Type, MaterialDeviceHandle> materials;
        vkb::Buffer materialBuffer;

        vk::DescriptorBufferInfo matBufferDescInfo;
    };
} // namespace trc
