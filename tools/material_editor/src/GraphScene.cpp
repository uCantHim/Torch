#include "GraphScene.h"



auto GraphScene::makeNode(s_ptr<trc::ShaderFunction> func) -> NodeID
{
    const NodeID id = graph.makeNode();

    createSockets(id, graph, func->getType());
    layoutSockets(id, graph, layout);
    layout.nodeSize.emplace(id, vec2(0.0f), calcNodeSize(id, graph));

    return id;
}
