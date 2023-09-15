#include "GraphLayout.h"



void layoutSockets(NodeID node, const MaterialGraph& graph, GraphLayout& layout)
{
    vec2 socketPos{ graph::kSocketSpacingVertical, graph::kSocketSpacingVertical };
    for (const auto sock : graph.inputSockets.get(node))
    {
        layout.socketSize.emplace(sock, socketPos, graph::kSocketSize);
        socketPos.y += graph::kSocketSize.y + graph::kSocketSpacingVertical;
    }

    socketPos.x += (!graph.inputSockets.get(node).empty()) ? graph::kSocketSize.x
                                                           : -graph::kSocketSpacingVertical;
    socketPos.x += graph::kSocketSpacingHorizontal;
    socketPos.y = graph::kSocketSpacingVertical;
    for (const auto sock : graph.outputSockets.get(node))
    {
        layout.socketSize.emplace(sock, socketPos, graph::kSocketSize);
        socketPos.y += graph::kSocketSize.y + graph::kSocketSpacingVertical;
    }
}

auto calcNodeSize(NodeID node, const MaterialGraph& graph) -> vec2
{
    const auto& inputs = graph.inputSockets.get(node);
    const auto& outputs = graph.outputSockets.get(node);
    const size_t maxVerticalSockets = glm::max(inputs.size(), outputs.size());

    vec2 extent{ 0.0f, 0.0f };

    extent.y = maxVerticalSockets * graph::kSocketSize.y
             + (maxVerticalSockets + 1) * graph::kSocketSpacingVertical;
    extent.x = (!inputs.empty() ? (graph::kSocketSize.y + graph::kSocketSpacingVertical) : 0)
             + graph::kSocketSpacingHorizontal
             + (!outputs.empty() ? graph::kSocketSize.y + graph::kSocketSpacingVertical : 0);

    extent = glm::max(extent, graph::kMinNodeSize);

    return extent;
}

bool isInside(vec2 point, const Hitbox& hb)
{
    const vec2 ll = glm::min(hb.origin, hb.origin + hb.extent);
    const vec2 ur = glm::max(hb.origin, hb.origin + hb.extent);
    return point.x > ll.x && point.x < ur.x
        && point.y > ll.y && point.y < ur.y;
}
