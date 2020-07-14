#pragma once

#include <filesystem>
namespace fs = std::filesystem;

#include <vulkan/vulkan.hpp>
#include <vkb/MemoryPool.h>
#include <vkb/Buffer.h>

#include "Vertex.h"
#include "Asset.h"

namespace trc
{
    struct MeshData
    {
        std::vector<Vertex> vertices;
        std::vector<ui32> indices;
    };

    class Geometry : public vkb::VulkanStaticInitialization<Geometry>, public Asset
    {
        friend class vkb::VulkanStaticInitialization<Geometry>;
        static void vulkanStaticInit() {
            pool.setPhysicalDevice(vkb::VulkanBase::getDevice().getPhysicalDevice());
        }

    public:
        Geometry(const MeshData& data);

        auto getIndexBuffer() const noexcept -> vk::Buffer;
        auto getVertexBuffer() const noexcept -> vk::Buffer;

    private:
        vkb::DeviceLocalBuffer indexBuffer;
        vkb::DeviceLocalBuffer vertexBuffer;

        ui32 numIndices;

        static constexpr vk::DeviceSize CHUNK_SIZE{ 50000000 }; // 50 MB
        static inline vkb::MemoryPool pool{ CHUNK_SIZE };
    };
}
