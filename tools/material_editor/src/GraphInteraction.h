#pragma once

#include <optional>
#include <unordered_set>

#include "MaterialGraph.h"

struct GraphInteraction
{
    std::optional<NodeID> hoveredNode;
    std::unordered_set<NodeID> selectedNodes;

    std::optional<SocketID> hoveredSocket;
};
