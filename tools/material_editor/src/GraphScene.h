#pragma once

#include "MaterialGraph.h"
#include "GraphLayout.h"

struct GraphScene
{
    // Graph topology
    MaterialGraph graph;

    // Graph geometry
    GraphLayout layout;

    auto makeNode() -> NodeID
    {
        const NodeID id = graph.makeNode();
        layout.nodeSize.emplace(id, vec2(0.0f), calcNodeSize(id, graph));

        return id;
    }
};
