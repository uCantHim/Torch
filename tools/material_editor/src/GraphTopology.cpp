#include "GraphTopology.h"

#include "Util.h"



auto GraphTopology::makeNode(Node node) -> NodeID
{
    NodeID id{ nodeId.generate() };
    nodeInfo.emplace(id, node);
    inputSockets.emplace(id);
    outputSockets.emplace(id);

    return id;
}

auto GraphTopology::makeSocket(Socket newSock) -> SocketID
{
    assert(newSock.parentNode != NodeID::NONE && "A socket must have a parent node!");

    SocketID id{ socketId.generate() };
    socketInfo.emplace(id, newSock);

    return id;
}

void GraphTopology::linkSockets(SocketID a, SocketID b)
{
    link.emplace(a, b);
    link.emplace(b, a);
}

void GraphTopology::unlinkSockets(SocketID a)
{
    link.erase(link.get(a));
    link.erase(a);
}

void GraphTopology::removeNode(NodeID id)
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



void createSockets(NodeID node, GraphTopology& graph, const NodeDescription& desc)
{
    auto makeInputField = [&](SemanticType type) -> InputElement
    {
        switch (type)
        {
        case SemanticType::eFloat:
            return NumberInputField{};
            break;
        case SemanticType::eRgb:
        case SemanticType::eRgba:
            return ColorInputField{};
            break;
        default:
            assert(false && "Not implemented.");
        }
    };

    for (const auto& arg : desc.inputs)
    {
        auto sock = graph.makeSocket({ node, arg.name, arg.description });
        graph.inputSockets.get(node).emplace_back(sock);
    }
    for (const auto& outVal : desc.outputs)
    {
        auto sock = graph.makeSocket({ node, access(outVal, name), access(outVal, description) });
        graph.outputSockets.get(node).emplace_back(sock);
        graph.outputValue.emplace(sock, outVal);

        // Create a user input field for constant values
        if (auto constVal = try_get<ConstantValue>(outVal)) {
            graph.socketDecoration.emplace(sock, makeInputField(constVal->type));
        }
    }
}

auto getIthSocket(const GraphTopology& graph, NodeID node, ui32 index) -> SocketID
{
    return index < graph.inputSockets.get(node).size()
        ? graph.inputSockets.get(node).at(index)
        : graph.outputSockets.get(node).at(index - graph.inputSockets.get(node).size());
}

auto findSocketIndex(const GraphTopology& graph, SocketID socket) -> ui32
{
    const auto node = graph.socketInfo.get(socket).parentNode;
    const auto sockets = { graph.inputSockets.get(node), graph.outputSockets.get(node) };
    for (ui32 i = 0; auto sock : sockets | std::views::join)
    {
        if (sock == socket) return i;
        ++i;
    }

    throw std::logic_error("Socket not found in its parent node.");
}
