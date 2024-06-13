#include "trc/core/RenderGraph.h"

#include <algorithm>
#include <functional>
#include <ranges>
#include <stdexcept>

#include <trc_util/Assert.h>



namespace trc
{

void RenderGraph::insert(RenderStage::ID newStage)
{
    if (findLowestOrderIndex(newStage) != std::nullopt)
    {
        throw std::invalid_argument("[In RenderGraph::insert]: "
                                    "New stage is already present in the graph!");
    }

    if (orderedStages.empty()) {
        orderedStages.emplace_back();
    }
    orderedStages.front().emplace_back(newStage);
    headStages.emplace(newStage);
    stageDeps.try_emplace(newStage);
}

void RenderGraph::createOrdering(RenderStage::ID from, RenderStage::ID to)
{
    if (!stageDeps.contains(from)) {
        insert(from);
    }
    if (!stageDeps.contains(to)) {
        insert(to);
    }

    auto [it, success] = stageDeps.at(from).emplace(to);
    if (!success) {
        return;  // Dependency has already been established.
    }

    if (hasCycles()) {
        stageDeps.at(from).erase(it);  // cleanup
        throw std::invalid_argument("Cycle in graph!");
    }

    headStages.erase(to);  // the dependency destination is no longer a head stage
    recalcLayout();
}

auto RenderGraph::begin() const -> const_iterator
{
    return RenderGraphIterator::makeBegin(this);
}

auto RenderGraph::end() const -> const_iterator
{
    return RenderGraphIterator::makeEnd(this);
}

auto RenderGraph::findLowestOrderIndex(RenderStage::ID stage) -> std::optional<size_t>
{
    for (size_t i = 0; const auto& vec : orderedStages)
    {
        if (std::ranges::find(vec, stage) != vec.end()) {
            return i;
        }
        ++i;
    }

    return std::nullopt;
}

bool RenderGraph::hasCycles() const
{
    if (headStages.empty()) {
        return true;
    }

    using Set = std::unordered_set<RenderStage::ID>;
    std::function<bool(RenderStage::ID, Set)> visit = [&](RenderStage::ID stage, Set visited) {
        if (!visited.emplace(stage).second) {
            return true;
        }
        for (const auto dep : stageDeps.at(stage))
        {
            if (visit(dep, visited)) {
                return true;
            }
        }
        return false;
    };

    for (const auto head : headStages)
    {
        if (visit(head, {})) {
            return true;
        }
    }
    return false;
}

void RenderGraph::recalcLayout()
{
    assert(!hasCycles());

    std::unordered_map<RenderStage::ID, size_t> ranks;
    std::function<void(RenderStage::ID, size_t)> visit = [&](RenderStage::ID stage, size_t rank) {
        auto [it, success] = ranks.try_emplace(stage, rank);
        if (!success) {
            it->second = std::max(rank, it->second);
        }

        for (const auto dep : stageDeps.at(stage)) {
            visit(dep, rank + 1);
        }
    };

    for (const auto head : headStages) {
        visit(head, 0);
    }

    assert(!ranks.empty());
    const size_t maxRank = *std::ranges::max_element(std::views::values(ranks));
    orderedStages.clear();
    orderedStages.resize(maxRank + 1);

    for (const auto& [stage, rank] : ranks) {
        orderedStages.at(rank).emplace_back(stage);
    }
}



RenderGraphIterator::RenderGraphIterator(const RenderGraph* _graph)
    :
    graph(_graph),
    outer(graph->orderedStages.begin()),
    inner(outer != graph->orderedStages.end() ? outer->begin() : InnerIter{})
{
    assert(graph != nullptr);
}

auto RenderGraphIterator::operator*() -> RenderStage::ID
{
    return *inner;
}

auto RenderGraphIterator::operator->() -> const RenderStage::ID*
{
    return &*inner;
}

auto RenderGraphIterator::operator++() -> RenderGraphIterator&
{
    if (outer == graph->orderedStages.end())
    {
        throw std::out_of_range("[In RenderGraphIterator::operator++]: "
                                "Trying to increment an `end` iterator!");
    }

    ++inner;
    if (inner == outer->end())
    {
        ++outer;
        if (outer == graph->orderedStages.end()) {
            inner = InnerIter{};
        }
        else {
            inner = outer->begin();
        }
    }

    return *this;
}

bool RenderGraphIterator::operator==(const RenderGraphIterator& other) const
{
    if (graph == other.graph)
    {
        // `outer` must be the same if `inner` is the same
        assert(inner == other.inner ? outer == other.outer : true);
    }

    return other.graph == this->graph
        && other.outer == this->outer
        && other.inner == this->inner;
}

auto RenderGraphIterator::makeBegin(const RenderGraph* graph) -> RenderGraphIterator
{
    return { graph };
}

auto RenderGraphIterator::makeEnd(const RenderGraph* graph) -> RenderGraphIterator
{
    RenderGraphIterator iter{ graph };
    iter.outer = graph->orderedStages.end();
    iter.inner = InnerIter{};
    return iter;
}

} // namespace trc
