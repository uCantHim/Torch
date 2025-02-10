#include "GraphSerializer.h"

#include <stdexcept>
#include <unordered_map>

#include <trc/base/Logging.h>

#include "material_graph.pb.h"

#include "GraphScene.h"



void serializeGraph(const GraphScene& scene, std::ostream& os)
{
    const auto& graph = scene.graph;
    const auto& layout = scene.layout;

    std::unordered_map<NodeID, ui32> nodeIndices;
    graph::MaterialGraph out;

    // Serialize nodes
    for (const auto [node, info] : graph.nodeInfo.items())
    {
        const ui32 index = out.nodes_size();
        nodeIndices.try_emplace(node, index);

        out.add_nodes()->set_id(info.desc.id);
        const vec2 pos = layout.nodeSize.get(node).origin;
        out.add_node_pos_x(pos.x);
        out.add_node_pos_y(pos.y);
    }
    out.set_output_node(nodeIndices.at(graph.outputNode));

    // Serialize links
    for (const auto [from, to] : graph.link.items())
    {
        const auto srcNode = graph.socketInfo.get(from).parentNode;
        const auto dstNode = graph.socketInfo.get(to).parentNode;

        auto link = out.add_links();
        link->set_src_node_index(nodeIndices.at(srcNode));
        link->set_src_socket_index(findSocketIndex(graph, from));
        link->set_dst_node_index(nodeIndices.at(dstNode));
        link->set_dst_socket_index(findSocketIndex(graph, to));
    }

    if (!out.SerializeToOstream(&os)) {
        throw std::runtime_error("Unable to write serialized material graph to ostream.");
    }
}

auto parseGraph(std::istream& is) -> std::optional<GraphScene>
{
    graph::MaterialGraph in;
    if (!in.ParseFromIstream(&is)) {
        return std::nullopt;
    }

    GraphScene res;

    // Create nodes
    std::unordered_map<ui32, NodeID> nodeIds;
    for (i32 i = 0; const auto& node : in.nodes())
    {
        if (!getMaterialNodes().contains(node.id()))
        {
            trc::log::warn << "Unable to create node with ID \"" << node.id() << "\":"
                           << " This ID does not exist.";
            ++i;
            continue;
        }

        const auto id = res.makeNode(getMaterialNodes().at(node.id()));
        res.layout.nodeSize.get(id).origin = vec2(in.node_pos_x(i), in.node_pos_y(i));

        nodeIds.try_emplace(i, id);
        ++i;
    }
    res.graph.outputNode = nodeIds.at(in.output_node());

    // Create socket links
    for (const auto& link : in.links())
    {
        res.graph.linkSockets(
            getIthSocket(res.graph, nodeIds.at(link.src_node_index()), link.src_socket_index()),
            getIthSocket(res.graph, nodeIds.at(link.dst_node_index()), link.dst_socket_index())
        );
    }

    return res;
}
