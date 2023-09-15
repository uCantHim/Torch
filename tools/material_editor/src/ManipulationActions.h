#pragma once

#include <ranges>
#include <vector>

#include "GraphManipulator.h"
#include "GraphScene.h"

namespace action
{
    struct MultiAction : public GraphManipAction
    {
        MultiAction() = default;
        MultiAction(auto&& range) {
            std::move(std::begin(range), std::end(range), std::back_inserter(actions));
        }
        explicit MultiAction(std::vector<u_ptr<GraphManipAction>> actions)
            : actions(std::move(actions)) {}

        void apply(GraphScene& graph) override
        {
            for (auto& action : actions) {
                action->apply(graph);
            }
        }

        void undo(GraphScene& graph) override
        {
            for (auto& action : actions | std::views::reverse) {
                action->undo(graph);
            }
        }

        void addAction(u_ptr<GraphManipAction> action) {
            actions.emplace_back(std::move(action));
        }

        std::vector<u_ptr<GraphManipAction>> actions;
    };

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
            nodePos = graph.layout.nodeSize.get(node).origin;
            graph.removeNode(node);
        }
        void undo(GraphScene& graph) override {
            node = graph.makeNode(desc);
            graph.layout.nodeSize.get(node).origin = nodePos;
        }

        NodeID node;
        NodeDescription desc;
        vec2 nodePos;
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
