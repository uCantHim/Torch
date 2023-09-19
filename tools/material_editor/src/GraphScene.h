#pragma once

#include <optional>

#include "GraphTopology.h"
#include "MaterialNode.h"
#include "GraphInteraction.h"
#include "GraphLayout.h"

struct GraphScene
{
    // Graph topology
    GraphTopology graph;

    // Graph geometry
    GraphLayout layout;

    // Graph state regarding user interaction
    GraphInteraction interaction;

    auto makeNode(NodeDescription desc) -> NodeID;

    /**
     * @brief Remove a node and all its sockets from the graph and the layout
     */
    void removeNode(NodeID node);

    /**
     * @return pair<node, socket> A node and a socket, if any of the respective
     *                            entity is hovered. If a socket is returned,
     *                            then the node is the one to which the socket
     *                            belongs.
     */
    auto findHover(vec2 position) const -> std::pair<std::optional<NodeID>, std::optional<SocketID>>;

private:
    auto findHoveredNode(vec2 position) const -> std::optional<NodeID>;
};
