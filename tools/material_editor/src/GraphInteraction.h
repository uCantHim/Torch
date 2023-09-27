#pragma once

#include <optional>
#include <string>
#include <unordered_set>

#include "GraphTopology.h"
#include "GraphLayout.h"

struct GraphInteraction
{
    std::optional<NodeID> hoveredNode;
    std::optional<SocketID> hoveredSocket;
    std::unordered_set<NodeID> selectedNodes;

    std::optional<Hitbox> multiSelectBox;

    // Text input fields are used to input floating-point numbers
    Table<std::string, SocketID> textInputFields;
    Table<vec4, SocketID> colorInputFields;
};

void createInputFields(GraphInteraction& inter, NodeID node, const GraphTopology& graph);

/**
 * @brief Update output values of sockets from their input field decorations.
 */
void updateOutputValues(const GraphInteraction& in, GraphTopology& graph);
