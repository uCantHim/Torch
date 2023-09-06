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
};
