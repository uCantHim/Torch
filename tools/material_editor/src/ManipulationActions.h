#pragma once

#include "GraphManipulator.h"
#include "GraphScene.h"

namespace action
{
    struct CreateNode : public GraphManipAction
    {
        explicit CreateNode(NodeDescription desc) : desc(std::move(desc)) {}

        void apply(GraphScene& graph) override {
            createdNode = graph.makeNode(desc);
        }
        void undo(GraphScene& graph) override {
            graph.removeNode(createdNode);
        }

        NodeDescription desc;
        NodeID createdNode;
    };

    struct RemoveNode : public GraphManipAction
    {
        explicit RemoveNode(NodeID node) : node(node) {}

        void apply(GraphScene& graph) override {
            desc = graph.graph.nodeInfo.get(node).desc;
            graph.removeNode(node);
        }
        void undo(GraphScene& graph) override {
            node = graph.makeNode(desc);
        }

        NodeID node;
        NodeDescription desc;
    };

    struct MoveNode : public GraphManipAction
    {
        MoveNode(NodeID node, vec2 from, vec2 to) : node(node), from(from), to(to) {}

        void apply(GraphScene& graph) override {
            graph.layout.nodeSize.get(node).origin = to;
        }
        void undo(GraphScene& graph) override {
            graph.layout.nodeSize.get(node).origin = from;
        }

        NodeID node;
        vec2 from;
        vec2 to;
    };
} // namespace action
