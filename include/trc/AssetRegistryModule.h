#pragma once

#include "VulkanInclude.h"
#include "DescriptorCreateHelpers.h"

namespace trc
{
    struct AssetRegistryModuleCreateInfo
    {
        const vkb::Device& device;

        ui32 geoVertexBufBinding;
        ui32 geoIndexBufBinding;
        ui32 materialBufBinding;
        ui32 textureBinding;

        vk::BufferUsageFlags geometryBufferUsage{};

        bool enableRayTracing{ true };
    };

    class AssetRegistryModuleInterface
    {
    public:
        AssetRegistryModuleInterface() = default;
        virtual ~AssetRegistryModuleInterface() = default;

        virtual void update(vk::CommandBuffer cmdBuf) = 0;

        virtual auto getDescriptorLayoutBindings() -> std::vector<DescriptorLayoutBindingInfo> = 0;
        virtual auto getDescriptorUpdates() -> std::vector<vk::WriteDescriptorSet> = 0;
    };

    template<typename T>
    concept AssetRegistryModuleType = requires (T a)
    {
        typename T::Handle;
        std::derived_from<T, AssetRegistryModuleInterface>;
        std::constructible_from<T, const AssetRegistryModuleCreateInfo&>;

        // { a.add(data) } -> std::same_as<TypedAssetID<T>>;
        // { a.remove(id) };
        // { a.getHandle(id) } -> std::same_as<AssetHandle<T>>;
    };
} // namespace trc
