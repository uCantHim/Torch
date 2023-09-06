#pragma once

#include <vector>

#include <trc/base/FrameSpecificObject.h>
#include <trc/base/Buffer.h>

#include "GraphLayout.h"
#include "MaterialGraph.h"

/**
 * @brief A renderable representation of a material graph
 */
struct GraphRenderData
{
    void pushNode(vec2 pos, vec2 size, vec4 color);
    void pushSocket(vec2 pos, vec2 size, vec4 color);
    void pushLink(vec2 from, vec2 to);

    void clear();

    /** List of vec4{ posX, posY, sizeX, sizeY } */
    std::vector<vec4> nodeDimensions;
    std::vector<vec4> nodeColors;

    /** List of vec4{ posX, posY, sizeX, sizeY } */
    std::vector<vec4> socketDimensions;
    std::vector<vec4> socketColors;
};

/**
 * @brief Compile a material graph into a renderable representation
 */
auto buildRenderData(const MaterialGraph& graph, const GraphLayout& layout) -> GraphRenderData;

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
        uint32_t numNodes{ 0 };
        uint32_t numSockets{ 0 };

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
