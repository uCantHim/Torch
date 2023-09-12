#pragma once

#include "MaterialGraph.h"
#include "GraphLayout.h"

struct GraphScene
{
    // Graph topology
    MaterialGraph graph;

    // Graph geometry
    GraphLayout layout;

    auto makeNode(s_ptr<trc::ShaderFunction> func) -> NodeID;
};
