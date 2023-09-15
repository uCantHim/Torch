#pragma once

#include <optional>
#include <unordered_set>

#include "MaterialGraph.h"
#include "GraphLayout.h"

struct GraphInteraction
{
    std::optional<NodeID> hoveredNode;
    std::optional<SocketID> hoveredSocket;
    std::unordered_set<NodeID> selectedNodes;

    std::optional<Hitbox> multiSelectBox;
};
