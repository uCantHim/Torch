#pragma once

#include <stack>

#include "GraphScene.h"

/**
 * @brief An invertible mutating operation on a material graph
 */
class GraphManipAction
{
public:
    virtual ~GraphManipAction() noexcept = default;

    virtual void apply(GraphScene& graph) = 0;
    virtual void undo(GraphScene& graph) = 0;
};

/**
 * @brief A command-based interface for modifications to a material graph
 */
class GraphManipulator
{
public:
    explicit GraphManipulator(s_ptr<GraphScene> graph);

    void applyAction(u_ptr<GraphManipAction> action);

    void undoLastAction();
    void reapplyLastUndoneAction();

private:
    s_ptr<GraphScene> graph;

    std::stack<u_ptr<GraphManipAction>> actionHistory;
    std::stack<u_ptr<GraphManipAction>> undoneActionHistory;
};
