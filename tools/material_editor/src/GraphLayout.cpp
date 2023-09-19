#include "GraphLayout.h"



void layoutSockets(NodeID node, const GraphTopology& graph, GraphLayout& layout)
{
    const vec2 nodeSize = layout.nodeSize.get(node).extent;

    // Layout input sockets
    vec2 socketPos = graph::kNodeContentStart;
    for (const auto sock : graph.inputSockets.get(node))
    {
        layout.socketSize.emplace(sock, socketPos, graph::kSocketSize);
        socketPos.y += graph::kSocketSize.y + graph::kPadding;
    }

    // Layout output socket
    socketPos.x = nodeSize.x - graph::kPadding - graph::kSocketSize.x;
    socketPos.y = graph::kNodeContentStart.y;
    for (const auto sock : graph.outputSockets.get(node))
    {
        layout.socketSize.emplace(sock, socketPos, graph::kSocketSize);
        socketPos.y += graph::kSocketSize.y + graph::kPadding;
    }
}

auto calcNodeSize(NodeID node, const GraphTopology& graph) -> vec2
{
    const auto& inputs = graph.inputSockets.get(node);
    const auto& outputs = graph.outputSockets.get(node);
    const size_t maxVerticalSockets = glm::max(inputs.size(), outputs.size());

    vec2 extent{ 0.0f, 0.0f };

    extent.y = graph::kNodeHeaderHeight
             + maxVerticalSockets * graph::kSocketSize.y
             + (maxVerticalSockets + 1) * graph::kPadding;
    extent.x = (!inputs.empty() ? (graph::kSocketSize.y + graph::kPadding) : 0)
             + graph::kSocketSpacingHorizontal
             + (!outputs.empty() ? graph::kSocketSize.y + graph::kPadding : 0);

    extent = glm::max(extent, graph::kMinNodeSize);

    return extent;
}

auto calcTitleTextPos(NodeID node, const GraphLayout& layout) -> vec2
{
    return layout.nodeSize.get(node).origin + vec2(graph::kPadding);
}

auto calcSocketGlobalPos(SocketID socket, const GraphTopology& graph, const GraphLayout& layout)
    -> vec2
{
    return layout.socketSize.get(socket).origin
         + layout.nodeSize.get(graph.socketInfo.get(socket).parentNode).origin;
}

bool isInside(vec2 point, const Hitbox& hb)
{
    const vec2 ll = glm::min(hb.origin, hb.origin + hb.extent);
    const vec2 ur = glm::max(hb.origin, hb.origin + hb.extent);
    return point.x > ll.x && point.x < ur.x
        && point.y > ll.y && point.y < ur.y;
}
