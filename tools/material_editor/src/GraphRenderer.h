#pragma once

#include <vector>

#include <trc/base/FrameSpecificObject.h>
#include <trc/base/Buffer.h>

#include "GraphScene.h"
#include "MaterialGraph.h"

namespace graph
{
    constexpr vec4 kNodeColor{ 0.025f, 0.025f, 0.025f, 1.0f };
    constexpr vec4 kSocketColor{ 0.8f, 0.2f, 0.2f, 1.0f };
    constexpr vec4 kLinkColor{ 0.8f, 0.8f, 0.8f, 1.0f };

    constexpr vec4 kSeparatorColor{ vec3(kNodeColor) * 3.0f, 1.0f };

    constexpr vec4 kHighlightColor{ 1.0f, 0.7f, 0.2f, 1.0f };
    constexpr vec4 kSelectColor{ 1.0f, 1.0f, 1.0f, 1.0f };

    // Color multiplier to highlight things by brightnening them
    constexpr float kHighlightColorModifier{ 1.8f };

    /**
     * Apply a multiplicative modifier to a color, keeping the color channels
     * in the bounds [0, 1].
     */
    constexpr auto applyColorModifier(auto color, float mod) -> decltype(color)
    {
        using vec = decltype(color);
        return glm::clamp(color * mod, vec(0.0f), vec(1.0f));
    }
} // namespace graph

/**
 * @brief A renderable representation of a material graph
 */
struct GraphRenderData
{
    void pushNode(vec2 pos, vec2 size, vec4 color);
    void pushSocket(vec2 pos, vec2 size, vec4 color);
    void pushLink(vec2 from, vec2 to, vec4 color);

    void pushBorder(vec2 pos, vec2 size, vec4 color);
    void pushSeparator(vec2 pos, float size, vec4 color);

    void clear();

    enum class Geometry { eQuad, eLine };

    struct Primitive
    {
        Geometry geometryType;

        /** List of vec4{ posX, posY, sizeX, sizeY } */
        std::vector<vec4> dimensions{};
        std::vector<vec4> colors{};
    };

    Primitive quads{ .geometryType=Geometry::eQuad };
    Primitive lines{ .geometryType=Geometry::eLine };
};

/**
 * @brief Compile a material graph into a renderable representation
 */
auto buildRenderData(const GraphScene& graph) -> GraphRenderData;

class MaterialGraphRenderer
{
public:
    MaterialGraphRenderer(const trc::Device& device, const trc::FrameClock& clock);

    void uploadData(const GraphRenderData& data);
    void draw(vk::CommandBuffer cmdBuf, trc::RenderConfig& conf);

private:
    static constexpr size_t kVertexPosOffset{ 0 };
    static constexpr size_t kVertexPosSize{ sizeof(vec2) * 6 };
    static constexpr size_t kVertexUvOffset{ kVertexPosSize };
    static constexpr size_t kVertexUvSize{ sizeof(vec2) * 6 };

    static constexpr size_t kElemDimSize{ sizeof(vec4) };
    static constexpr size_t kElemColorSize{ sizeof(vec4) };

    struct DeviceData
    {
        struct DrawCmd
        {
            ui32 indexCount;
            ui32 instanceCount;
            trc::Pipeline::ID pipeline;
        };

        std::vector<DrawCmd> drawCmds;

        trc::Buffer dimensionBuf;
        trc::Buffer colorBuf;

        vec4* dimensions;
        vec4* colors;
    };

    const trc::Device& device;

    trc::DeviceLocalBuffer vertexBuf;
    trc::DeviceLocalBuffer indexBuf;
    trc::FrameSpecific<DeviceData> deviceData;
};
