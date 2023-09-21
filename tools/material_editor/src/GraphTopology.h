#pragma once

#include <string>
#include <vector>

#include <componentlib/Table.h>
#include <trc/Types.h>
#include <trc/material/BasicType.h>
#include <trc/material/ShaderModuleBuilder.h>
#include <trc_util/data/IdPool.h>
#include <trc_util/data/TypesafeId.h>

#include "MaterialNode.h"

using namespace trc::basic_types;

struct Socket;
struct Node;

using SocketID = trc::data::TypesafeID<Socket>;
using NodeID = trc::data::TypesafeID<Node>;
using componentlib::Table;

struct Node
{
    NodeDescription desc;
};

struct Socket
{
    NodeID parentNode;
    ArgDescription desc;
};

/**
 * @brief Topological information about a material graph
 */
struct GraphTopology
{
    NodeID outputNode;

    Table<Node, NodeID> nodeInfo;

    Table<std::vector<SocketID>, NodeID> inputSockets;
    Table<std::vector<SocketID>, NodeID> outputSockets;

    /**
     * Every valid socket ID has an associated entry in the `socketInfo` table.
     */
    Table<Socket, SocketID> socketInfo;

    /**
     * Sockets only have an entry here if they are output sockets.
     */
    Table<NodeComputation, SocketID> outputComputation;

    /**
     * Links between sockets are defined as entries in the `link` table. If a
     * socket has no entry here, it is not linked to another socket.
     */
    Table<SocketID, SocketID> link;

    auto makeNode(Node node) -> NodeID;
    auto makeSocket(Socket newSock) -> SocketID;

    void linkSockets(SocketID a, SocketID b);
    void unlinkSockets(SocketID a);

    /**
     * @brief Remove a node and all associated objects from the graph
     *
     * Removes a node from the graph. Removes all of the node's sockets. Removes
     * all links from/to any of the node's sockets.
     */
    void removeNode(NodeID id);

private:
    trc::data::IdPool<uint32_t> nodeId;
    trc::data::IdPool<uint32_t> socketId;
};

/**
 * @brief Create sockets for a node based on a function signature
 */
void createSockets(NodeID node, GraphTopology& graph, const NodeDescription& desc);

/**
 * @return SocketID The `index`-th socket, counting both input and output
 *                  sockets.
 */
auto getIthSocket(const GraphTopology& graph, NodeID node, ui32 index) -> SocketID;

/**
 * @brief Find a socket's index in its parent node's set of sockets
 */
auto findSocketIndex(const GraphTopology& graph, SocketID socket) -> ui32;

bool isOutputSocket(const GraphTopology& graph, SocketID socket);
