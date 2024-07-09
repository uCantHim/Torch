#pragma once

#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "trc/core/RenderStage.h"

namespace trc
{
    /**
     * @brief An ordered, iterable representation of a render graph.
     *
     * Create a render graph layout with `RenderGraph::compile`.
     *
     * Iterating over the render graph layout yields defined render stages in
     * the order in which they depend on each other.
     */
    class RenderGraphLayout;

    /**
     * @brief Declares render stages and dependencies between them.
     */
    class RenderGraph
    {
    public:
        RenderGraph() = default;

        /**
         * @brief Add a stage to the graph.
         */
        void insert(RenderStage::ID newStage);

        /**
         * @brief Insert a dependency edge into the graph.
         *
         * Any stage that does not yet exist in the graph is inserted first.
         *
         * @throw std::invalid_argument if the new edge results in a cycle.
         */
        void createOrdering(RenderStage::ID from, RenderStage::ID to);

        /**
         * @throw std::runtime_error if the graph contains cycles.
         */
        auto compile() const -> RenderGraphLayout;

    private:
        /**
         * @return bool True if the graph contains a cycle, false otherwise.
         */
        bool hasCycles() const;

        // Declares all dependencies among stages. Is not inherently cycle-free;
        // `hasCycles` computes that property.
        std::unordered_map<RenderStage::ID, std::unordered_set<RenderStage::ID>> stageDeps;

        // Stages that don't depend on any other stages.
        std::unordered_set<RenderStage::ID> headStages;
    };

    class RenderGraphLayout
    {
    public:
        using const_iterator = class RenderGraphIterator;

        /**
         * @return size_t Number of render stages in the graph.
         */
        auto size() const -> size_t;

        /** Iterate over stages in the graph in their dependency order. */
        auto begin() const -> const_iterator;

        /** Iterate over stages in the graph in their dependency order. */
        auto end() const -> const_iterator;

    private:
        friend RenderGraph;                // Creates RenderGraphLayout
        friend class RenderGraphIterator;  // Accesses RenderGraphLayout's private member

        RenderGraphLayout() = default;

        /**
         * As we are only interested in the acyclic ordering of stages with
         * respect to each other, I constructed an algorithm that distributes
         * stages into 'ranks' according to the dependencies specified via
         * `createOrdering`.
         *
         * Example render graph:
         *
         *   | Rank 0  | Rank 1  | Rank 2  | Rank 3
         *   |         |         |         |
         *   |---------|---------|---------|-------
         *   |         |      /--|----o    |
         *   |    o----|-\   /   |         |
         *   |         |  |-o----|----o----|---o
         *   |    o----|-/   \   |         |
         *   |         |      \--|----o    |
         *   |         |         |   /     |
         *   |    o----|---------|--/      |
         *
         * `orderedStages[0]` contains all nodes of rank 0, and so on.
         */
        std::vector<std::vector<RenderStage::ID>> orderedStages;
    };

    class RenderGraphIterator
    {
    public:
        using iterator_category = std::forward_iterator_tag;

        auto operator*() -> RenderStage::ID;
        auto operator->() -> const RenderStage::ID*;

        auto operator++() -> RenderGraphIterator&;

        bool operator==(const RenderGraphIterator& other) const;

        static auto makeBegin(const RenderGraphLayout& graph) -> RenderGraphIterator;
        static auto makeEnd(const RenderGraphLayout& graph) -> RenderGraphIterator;

    private:
        using OuterIter = decltype(RenderGraphLayout::orderedStages)::const_iterator;
        using InnerIter = std::vector<RenderStage::ID>::const_iterator;

        RenderGraphIterator(const RenderGraphLayout& graph);

        const RenderGraphLayout& graph;
        OuterIter outer;
        InnerIter inner;
    };
} // namespace trc
