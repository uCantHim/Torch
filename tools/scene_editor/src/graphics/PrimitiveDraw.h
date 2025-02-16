#pragma once

#include <span>
#include <vector>

#include <trc/Torch.h>

using namespace trc::basic_types;

namespace internal
{
    struct Vertex2D
    {
        vec2 pos;
        vec4 color;
    };
} // namespace internal

class PrimitiveDrawList
{
public:
    void pushLine(vec2 from, vec2 to, vec4 color);
    void pushQuad(vec2 ll, vec2 ur, vec4 color);

    auto getLineVerts() const -> std::span<const internal::Vertex2D>;
    auto getTriangleVerts() const -> std::span<const internal::Vertex2D>;

    void clear();

private:
    std::vector<internal::Vertex2D> lineVerts;
    std::vector<internal::Vertex2D> triangleVerts;
};

class PrimitiveRenderer
{
public:
    PrimitiveRenderer(trc::Device* device,
                      vk::Format renderTargetFormat,
                      const trc::DeviceMemoryAllocator& alloc);

    void draw(const PrimitiveDrawList& drawList,
              trc::Frame& frame,
              trc::RenderStage stage,
              const trc::Viewport& vp);
private:
    struct DrawCommand
    {
        trc::Pipeline& pipeline;

        vk::Buffer vertexBuffer;
        size_t vertexBufferOffset;
        size_t numVertices;
    };

    auto uploadDrawData(const PrimitiveDrawList& drawList) -> std::vector<DrawCommand>;

    const trc::Device* device;
    const vk::Format renderTargetFormat;
    const trc::DeviceMemoryAllocator alloc;

    trc::PipelineLayout pipelineLayout;
    trc::PipelineTemplate pipelineBase;
    trc::Pipeline linePipeline;
    trc::Pipeline trianglePipeline;

    trc::Buffer vertexBuffer;
};
