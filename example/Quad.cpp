#include "Quad.h"

#include <glm/gtc/matrix_transform.hpp>

#include "Vertex.h"



auto makeQuadVertices()
{
    std::vector<Vertex> result = {
        { vec3(0, 0, 0), vec3(), vec2() },
        { vec3(1, 0, 0), vec3(), vec2() },
        { vec3(0, 1, 0), vec3(), vec2() },
        { vec3(1, 1, 0), vec3(), vec2() },
    };

    return result;
}

auto makeQuadIndices()
{
    std::vector<uint32_t> result = {
        0, 1, 3,
        0, 3, 2,
    };

    return result;
}

Quad::Quad()
    :
    commandBuffer([](uint32_t) {
        return vkb::VulkanBase::getDevice().createGraphicsCommandBuffer(
            vk::CommandBufferLevel::eSecondary
        );
    }),
    geometryBuffer([]() {
        auto verts = makeQuadVertices();
        return vkb::DeviceLocalBuffer{
            sizeof(Vertex) * verts.size(),
            verts.data(),
            vk::BufferUsageFlagBits::eVertexBuffer
        };
    }()),
    indexBuffer([]() {
        auto indices = makeQuadIndices();
        return vkb::DeviceLocalBuffer{
            sizeof(uint32_t) * indices.size(),
            indices.data(),
            vk::BufferUsageFlagBits::eIndexBuffer
        };
    }())
{
}

auto Quad::recordCommandBuffer(uint32_t subpass, const vk::CommandBufferInheritanceInfo& inheritInfo)
    -> vk::CommandBuffer
{
    auto cmdBuf = **commandBuffer;

    cmdBuf.begin({
        vk::CommandBufferUsageFlagBits::eRenderPassContinue | vk::CommandBufferUsageFlagBits::eOneTimeSubmit,
        &inheritInfo
    });

    getPipeline(subpass).bind(cmdBuf);
    cmdBuf.bindVertexBuffers(
        0, // First binding
        geometryBuffer.get(),
        vk::DeviceSize(0) // offset
    );
    cmdBuf.bindIndexBuffer(
        indexBuffer.get(),
        0, // offset
        vk::IndexType::eUint32
    );

    mat4 model = glm::translate(mat4(1.0f), vec3(-1, 0, -4));
    cmdBuf.pushConstants<mat4>(
        getPipeline(subpass).getPipelineLayout(),
        vk::ShaderStageFlagBits::eVertex,
        0,
        model
    );

    cmdBuf.drawIndexed(
        6, // index count
        1, // instance count
        0, // first index
        0, // vertex offset
        0  // first instance
    );
    cmdBuf.end();

    return cmdBuf;
}
