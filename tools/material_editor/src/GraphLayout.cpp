#include "GraphLayout.h"



auto calcNodeSize(NodeID node, const MaterialGraph& graph) -> vec2
{
    const auto& inputs = graph.inputSockets.get(node);
    const auto& outputs = graph.outputSockets.get(node);
    const size_t maxVerticalSockets = glm::max(inputs.size(), outputs.size());

    vec2 extent{ 0.0f, 0.0f };

    extent.y = maxVerticalSockets * graph::kSocketSize.y
                + (maxVerticalSockets - 1) * graph::kSocketSpacingVertical;
    extent.x = static_cast<float>(!inputs.empty()) * graph::kSocketSize.y
                + graph::kSocketSpacingHorizontal
                + static_cast<float>(!outputs.empty()) * graph::kSocketSize.y;

    extent = glm::max(extent, graph::kMinNodeSize);

    return extent;
}

bool isInside(vec2 point, const Hitbox& hb)
{
    return point.x > hb.origin.x && point.x < hb.origin.x + hb.extent.x
        && point.y > hb.origin.y && point.y < hb.origin.y + hb.extent.y;
}
