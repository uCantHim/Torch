#pragma once

#include <filesystem>
namespace fs = std::filesystem;

#include <vulkan/vulkan.hpp>
#include <vkb/MemoryPool.h>
#include <vkb/Buffer.h>

#include "Asset.h"
#include "Vertex.h"
#include "Rig.h"

namespace trc
{
    struct MeshData
    {
        std::vector<Vertex> vertices;
        std::vector<ui32> indices;
    };

    class Geometry : public vkb::VulkanStaticInitialization<Geometry>
                   , public vkb::VulkanStaticDestruction<Geometry>
                   , public Asset
    {
        friend class vkb::VulkanStaticInitialization<Geometry>;
        friend class vkb::VulkanStaticDestruction<Geometry>;
        static void vulkanStaticInit() {
            pool.setDevice(vkb::VulkanBase::getDevice());
        }
        static void vulkanStaticDestroy() {
            pool.reset();
        }

    public:
        Geometry(const MeshData& data);
        Geometry(const MeshData& data, std::unique_ptr<Rig> rig);

        auto getIndexBuffer() const noexcept -> vk::Buffer;
        auto getVertexBuffer() const noexcept -> vk::Buffer;
        auto getIndexCount() const noexcept -> ui32;

        auto hasRig() const noexcept -> bool;
        auto getRig() const noexcept -> Rig*;

    private:
        vkb::DeviceLocalBuffer indexBuffer;
        vkb::DeviceLocalBuffer vertexBuffer;

        ui32 numIndices;
        std::unique_ptr<Rig> rig{ nullptr };

        static constexpr vk::DeviceSize CHUNK_SIZE{ 50000000 }; // 50 MB
        static inline vkb::MemoryPool pool{ CHUNK_SIZE };
    };
} // namespace trc
