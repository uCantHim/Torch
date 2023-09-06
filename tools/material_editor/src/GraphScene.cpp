#include "GraphScene.h"

#include <ranges>



auto GraphScene::makeNode(NodeDescription desc) -> NodeID
{
    const NodeID id = graph.makeNode({ desc });

    // Create objects
    createSockets(id, graph, desc);

    // Layout objects
    layout.nodeSize.emplace(id, vec2(0.0f), calcNodeSize(id, graph));
    layoutSockets(id, graph, layout);

    return id;
}

void GraphScene::removeNode(NodeID node)
{
    // Remove from layout
    auto sockets = { graph.inputSockets.get(node), graph.outputSockets.get(node) };
    for (auto sock : sockets | std::views::join) {
        layout.socketSize.erase(sock);
    }
    layout.nodeSize.erase(node);

    // Remove from interaction info
    if (interaction.hoveredNode == node) {
        interaction.hoveredNode.reset();
    }
    interaction.selectedNodes.erase(node);

    // Remove from graph topology
    graph.removeNode(node);
}

auto GraphScene::findHoveredNode(const vec2 pos) const -> std::optional<NodeID>
{
    for (const auto [node, hitbox] : layout.nodeSize.items())
    {
        if (isInside(pos, hitbox)) {
            return node;
        }
    }

    return std::nullopt;
}

auto GraphScene::findHover(const vec2 pos) const -> GraphHoverInfo
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
                return { node, sock, std::nullopt };
            }

            // Now test for hover of the socket's decoration element
            if (auto decoration = layout.decorationSize.try_get(sock))
            {
                const auto& hitbox = decoration->get();
                if (isInside(pos, { hitbox.origin + nodePos, hitbox.extent })) {
                    return { node, std::nullopt, sock };
                }
            }
        }

        return { node, std::nullopt, std::nullopt };
    }

    return { std::nullopt, std::nullopt, std::nullopt };
}
