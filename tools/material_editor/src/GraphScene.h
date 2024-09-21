#pragma once

#include <optional>

#include <trc/core/SceneModule.h>

#include "GraphTopology.h"
#include "MaterialNode.h"
#include "GraphInteraction.h"
#include "GraphLayout.h"

struct GraphHoverInfo
{
    std::optional<NodeID> hoveredNode;
    std::optional<SocketID> hoveredSocket;
    std::optional<SocketID> hoveredInputField;
};

struct GraphScene : public trc::SceneModule
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
    auto findHover(vec2 position) const -> GraphHoverInfo;

private:
    auto findHoveredNode(vec2 position) const -> std::optional<NodeID>;
};
