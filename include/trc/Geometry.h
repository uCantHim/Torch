#pragma once

#include <filesystem>
namespace fs = std::filesystem;

#include <vulkan/vulkan.hpp>
#include <vkb/MemoryPool.h>
#include <vkb/Buffer.h>

#include "Asset.h"
#include "Vertex.h"

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
            pool.setDevice(vkb::VulkanBase::getDevice());
        }

    public:
        Geometry(const MeshData& data);

        auto getIndexBuffer() const noexcept -> vk::Buffer;
        auto getVertexBuffer() const noexcept -> vk::Buffer;

        auto getIndexCount() const noexcept -> ui32;

    private:
        vkb::DeviceLocalBuffer indexBuffer;
        vkb::DeviceLocalBuffer vertexBuffer;

        ui32 numIndices;

        static constexpr vk::DeviceSize CHUNK_SIZE{ 50000000 }; // 50 MB
        static inline vkb::MemoryPool pool{ CHUNK_SIZE };
    };
}
