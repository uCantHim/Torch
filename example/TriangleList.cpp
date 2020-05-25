#include "TriangleList.h"

#include <random>

#include <glm/gtc/matrix_transform.hpp>

#include "Vertex.h"



TriangleList::TriangleList(size_t vertexCount)
    :
    cmdBuf([](uint32_t) {
        return vkb::VulkanBase::getDevice().createGraphicsCommandBuffer(
            vk::CommandBufferLevel::eSecondary
        );
    }),
    vertexCount(vertexCount),
    vertexBuffer(
        [&]() {
            std::vector<Vertex> vertices;

            std::random_device d;
            std::mt19937 engine(d());
            std::mt19937 engine2(d());
            std::uniform_real_distribution<float> rn(-1.0f, 1.0f);
            std::uniform_real_distribution<float> uv(0.0f, 1.0f);

            vertices.resize(vertexCount);
            for (size_t i = 0; i < vertexCount; i++)
            {
                vertices[i] = {
                    vec3(rn(engine), rn(engine), rn(engine)),
                    normalize(vec3(rn(engine))),
                    vec2(uv(engine2), uv(engine2))
                };
            }

            return vkb::DeviceLocalBuffer(
                sizeof(Vertex) * vertices.size(),
                vertices.data(),
                vk::BufferUsageFlagBits::eVertexBuffer
            );
        }()
    )
{
}


auto TriangleList::recordCommandBuffer(uint32_t subpass, const vk::CommandBufferInheritanceInfo& inheritInfo)
    -> vk::CommandBuffer
{
    cmdBuf.get()->begin(
        vk::CommandBufferBeginInfo(
            vk::CommandBufferUsageFlagBits::eRenderPassContinue,
            &inheritInfo
        )
    );

    /**
     * TODO:
     *
     * Group pipeline bind and descriptor set bind into one method of the
     * pipeline.
     * Group command buffer collects of objects with the same pipeline together
     * in command collectors and call the pipeline/descriptor set bind method
     * only once
     */

    cmdBuf.get()->bindPipeline(vk::PipelineBindPoint::eGraphics, *getPipeline(subpass));
    cmdBuf.get()->bindDescriptorSets(
        vk::PipelineBindPoint::eGraphics,
        getPipeline(subpass).getPipelineLayout(),
        0, // First set
        getPipeline(subpass).getPipelineDescriptorSets(),
        {}
    );
    cmdBuf.get()->bindVertexBuffers(
        INPUT_BINDING_0,
        vertexBuffer.get(),
        vk::DeviceSize(0) // offset
    );

    mat4 model = glm::translate(mat4(1.0f), vec3(-1, 0, -2));
    cmdBuf.get()->pushConstants<mat4>(
        getPipeline(subpass).getPipelineLayout(),
        vk::ShaderStageFlagBits::eVertex,
        0,
        model
    );

    cmdBuf.get()->draw(static_cast<uint32_t>(vertexCount), 1, 0, 0);
    cmdBuf.get()->end();

    return **cmdBuf;
}
