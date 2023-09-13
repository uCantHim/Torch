#include "GraphScene.h"

#include <ranges>



auto GraphScene::makeNode(s_ptr<trc::ShaderFunction> func) -> NodeID
{
    const NodeID id = graph.makeNode();

    createSockets(id, graph, func->getType());
    layoutSockets(id, graph, layout);
    layout.nodeSize.emplace(id, vec2(0.0f), calcNodeSize(id, graph));

    return id;
}

auto GraphScene::findHoveredNode(const vec2 pos) const -> std::optional<NodeID>
{
    for (const auto node : graph.nodeInfo.keys())
    {
        if (isInside(pos, layout.nodeSize.get(node))) {
            return node;
        }
    }

    return std::nullopt;
}

auto GraphScene::findHover(const vec2 pos) const
    -> std::pair<std::optional<NodeID>, std::optional<SocketID>>
{
    // Node hitboxes act as a broadphase for socket hitboxes
    if (auto node = findHoveredNode(pos))
    {
        const auto sockets = { graph.inputSockets.get(*node), graph.outputSockets.get(*node) };

        const vec2 nodePos = layout.nodeSize.get(*node).origin;
        for (const SocketID sock : sockets | std::views::join)
        {
            const auto& hitbox = layout.socketSize.get(sock);
            if (isInside(pos, { hitbox.origin + nodePos, hitbox.extent })) {
                return { node, sock };
            }
        }

        return { node, std::nullopt };
    }

    return { std::nullopt, std::nullopt };
}
