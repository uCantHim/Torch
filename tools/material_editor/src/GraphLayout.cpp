#include "GraphLayout.h"

#include "Font.h"



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
        if (graph.socketDecoration.contains(sock))
        {
            auto pos = socketPos;
            pos.x -= graph::kTextInputFieldSize.x + graph::kPadding;
            layout.decorationSize.emplace(sock, pos, graph::kTextInputFieldSize);
        }
        socketPos.y += graph::kSocketSize.y + graph::kPadding;
    }
}

auto calcSocketSize(SocketID sock, const GraphTopology& graph) -> vec2
{
    vec2 size = graph::kSocketSize;
    if (graph.socketDecoration.contains(sock)) {
        size.x += graph::kTextInputFieldSize.x + graph::kPadding;
    }

    return size;
}

auto calcNodeSize(NodeID node, const GraphTopology& graph) -> vec2
{
    using namespace std::placeholders;

    const auto& inputs = graph.inputSockets.get(node);
    const auto& outputs = graph.outputSockets.get(node);
    const size_t maxVerticalSockets = glm::max(inputs.size(), outputs.size());

    const auto _calcSocketSize = std::bind(calcSocketSize, _1, std::ref(graph));
    auto inputSizes = inputs | std::views::transform(_calcSocketSize)
                             | std::views::transform([](auto&& v){ return v.x; });
    auto outputSizes = outputs | std::views::transform(_calcSocketSize)
                               | std::views::transform([](auto&& v){ return v.x; });

    // Calculate node size to fit its sockets
    vec2 extent{ 0.0f, 0.0f };
    extent.y = graph::kNodeHeaderHeight
             + static_cast<float>(maxVerticalSockets) * graph::kSocketSize.y
             + static_cast<float>(maxVerticalSockets + 1) * graph::kPadding;
    extent.x = (!inputs.empty() ? (*std::ranges::max_element(inputSizes) + graph::kPadding) : 0)
             + graph::kSocketSpacingHorizontal
             + (!outputs.empty() ? (*std::ranges::max_element(outputSizes) + graph::kPadding) : 0);

    // Possibly rescale node to fit its title text in width
    const float nodeTitleWidth = calcTextSize(graph.nodeInfo.get(node).desc.name,
                                              graph::kTextHeight,
                                              getTextFont()).x;
    extent.x = glm::max(extent.x, nodeTitleWidth + graph::kPadding * 2.0f);

    // Fallback constraint in case something weird happens
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
