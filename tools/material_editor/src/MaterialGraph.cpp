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

void MaterialGraph::removeNode(NodeID id)
{
    if (id == outputNode) {
        throw std::invalid_argument("[In MaterialGraph::removeNode]: Unable to remove the"
                                    " output node!");
    }

    /** Remove a socket and its link (if it has any) from the graph */
    auto removeSocket = [this](SocketID sock) {
        assert(!link.contains(sock) ^ (link.contains(sock) && !link.contains(link.get(sock)))
               && "The material graph must be unidirectional!");
        link.try_erase(sock);
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
