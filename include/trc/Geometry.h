#pragma once

#include <filesystem>
namespace fs = std::filesystem;

#include <vulkan/vulkan.hpp>
#include <vkb/Buffer.h>

#include "Vertex.h"

namespace trc
{
    struct MeshData
    {
        std::vector<Vertex> vertices;
        std::vector<ui32> indices;
    };

    class Geometry
    {
    public:
        explicit Geometry(const MeshData& data);

        auto getIndexBuffer() const noexcept -> vk::Buffer;
        auto getVertexBuffer() const noexcept -> vk::Buffer;

    private:
        vkb::DeviceLocalBuffer indexBuffer;
        vkb::DeviceLocalBuffer vertexBuffer;

        ui32 numIndices;
    };
}