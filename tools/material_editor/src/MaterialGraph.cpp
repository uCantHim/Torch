#include "MaterialGraph.h"



auto MaterialGraph::makeNode(Node node) -> NodeID
{
    NodeID id{ nodeId.generate() };
    nodeInfo.emplace(id, node);
    inputSockets.emplace(id);
    outputSockets.emplace(id);

    return id;
}

auto MaterialGraph::makeSocket(Socket newSock) -> SocketID
{
    assert(newSock.parentNode != NodeID::NONE && "A socket must have a parent node!");

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
    nodeInfo.erase(id);
}



void createSockets(NodeID node, MaterialGraph& graph, const NodeDescription& desc)
{
    for (const auto& arg : desc.inputs)
    {
        auto sock = graph.makeSocket({ .parentNode=node, .desc=arg });
        graph.inputSockets.get(node).emplace_back(sock);
    }
    if (desc.output)
    {
        auto sock = graph.makeSocket({ .parentNode=node, .desc=*desc.output });
        graph.outputSockets.get(node).emplace_back(sock);
    }
}
