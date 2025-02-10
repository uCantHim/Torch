#include "GraphManipulator.h"



auto pop_from(auto&& container)
{
    auto top = std::move(container.top());
    container.pop();
    return top;
}



GraphManipulator::GraphManipulator(s_ptr<GraphScene> graph)
    :
    graph(graph)
{
}

void GraphManipulator::applyAction(u_ptr<GraphManipAction> action)
{
    action->apply(*graph);
    actionHistory.emplace(std::move(action));

    // Clear the history of re-applyable actions because the global order of
    // actions has been modified
    undoneActionHistory = {};
}

void GraphManipulator::undoLastAction()
{
    if (!actionHistory.empty())
    {
        auto action = pop_from(actionHistory);
        action->undo(*graph);
        undoneActionHistory.emplace(std::move(action));
    }
}

void GraphManipulator::reapplyLastUndoneAction()
{
    if (!undoneActionHistory.empty())
    {
        auto action = pop_from(undoneActionHistory);
        action->apply(*graph);
        actionHistory.emplace(std::move(action));
    }
}
