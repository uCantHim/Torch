#pragma once

#include <optional>

#include <vkb/Buffer.h>
#include <vkb/MemoryPool.h>
#include <trc_util/data/IndexMap.h>
#include <trc_util/data/ObjectId.h>

#include "AssetIds.h"
#include "Geometry.h"
#include "Rig.h"
#include "AssetRegistryModule.h"

namespace trc
{
    /**
     * @brief
     */
    class GeometryRegistry : public AssetRegistryModuleInterface
    {
    public:
        using LocalID = GeometryID::LocalID;
        using Handle = GeometryDeviceHandle;

        explicit GeometryRegistry(const AssetRegistryModuleCreateInfo& info);

        void update(vk::CommandBuffer cmdBuf) final;

        auto getDescriptorLayoutBindings() -> std::vector<DescriptorLayoutBindingInfo> final;
        auto getDescriptorUpdates() -> std::vector<vk::WriteDescriptorSet> final;

        auto add(const GeometryData& data) -> LocalID;
        void remove(LocalID id);

        auto getHandle(LocalID id) -> GeometryDeviceHandle;

    private:
        struct Config
        {
            vk::BufferUsageFlags geometryBufferUsage;
            bool enableRayTracing;

            ui32 vertBufBinding;
            ui32 indexBufBinding;
        };

        /**
         * GPU resources for geometry data
         */
        struct InternalStorage
        {
            using VertexType = GeometryDeviceHandle::VertexType;

            operator GeometryDeviceHandle()
            {
                return {
                    *indexBuf, numIndices, vk::IndexType::eUint32,
                    *vertexBuf, vertexType,
                    rig.has_value() ? &rig.value() : nullptr
                };
            }

            vkb::DeviceLocalBuffer indexBuf;
            vkb::DeviceLocalBuffer vertexBuf;
            ui32 numIndices{ 0 };
            ui32 numVertices{ 0 };

            VertexType vertexType;

            std::optional<Rig> rig;

            ui32 deviceIndex;
        };

        static constexpr ui32 MEMORY_POOL_CHUNK_SIZE = 200000000;  // 200 MiB
        static constexpr ui32 MAX_GEOMETRY_COUNT = 5000;

        const vkb::Device& device;
        const Config config;

        vkb::MemoryPool memoryPool;
        data::IdPool idPool;

        data::IndexMap<LocalID::IndexType, InternalStorage> storage;
    };
} // namespace trc
