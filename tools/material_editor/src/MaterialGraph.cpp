#include "MaterialGraph.h"



auto MaterialGraph::makeNode() -> NodeID
{
    NodeID id{ nodeId.generate() };
    nodeInfo.emplace(id);
    inputSockets.emplace(id);
    outputSockets.emplace(id);

    return id;
}

auto MaterialGraph::makeSocket(Socket newSock) -> SocketID
{
    SocketID id{ socketId.generate() };
    socketInfo.emplace(id, newSock);

    return id;
}

void MaterialGraph::linkSockets(SocketID a, SocketID b)
{
    link.emplace(a, b);
    link.emplace(b, a);
}

void MaterialGraph::unlinkSockets(SocketID a)
{
    link.erase(link.get(a));
    link.erase(a);
}

void MaterialGraph::removeNode(NodeID id)
{
    if (id == outputNode) {
        throw std::invalid_argument("[In MaterialGraph::removeNode]: Unable to remove the"
                                    " output node!");
    }

    /** Remove a socket and its link (if it has any) from the graph */
    auto removeSocket = [this](SocketID sock) {
        if (link.contains(sock))
        {
            const auto linkedSock = link.get(sock);
            assert(link.contains(linkedSock));
            link.erase(sock);
            link.erase(linkedSock);
        }
        socketInfo.erase(sock);
    };

    for (auto sock : inputSockets.get(id)) {
        removeSocket(sock);
    }
    for (auto sock : outputSockets.get(id)) {
        removeSocket(sock);
    }

    inputSockets.erase(id);
    outputSockets.erase(id);
}

void createSockets(NodeID node, MaterialGraph& graph, const trc::FunctionType& type)
{
    for (const auto& argType : type.argTypes)
    {
        auto sock = graph.makeSocket({ .type=argType, .name="arg_name" });
        graph.inputSockets.get(node).emplace_back(sock);
    }
    if (type.returnType)
    {
        auto sock = graph.makeSocket({ .type=type.returnType.value(), .name="ret_name" });
        graph.outputSockets.get(node).emplace_back(sock);
    }
}
