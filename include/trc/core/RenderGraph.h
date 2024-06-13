#pragma once

#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "trc/core/RenderStage.h"

namespace trc
{
    class Window;

    /**
     * @brief Declares render stages and dependencies between them.
     */
    class RenderGraph
    {
    public:
        friend class RenderGraphIterator;

        using const_iterator = class RenderGraphIterator;

        RenderGraph() = default;

        /**
         * @throw std::invalid_argument if `newStage` is already present in the
         *        render graph.
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

        /** Iterate over stages in the graph in their dependency order. */
        auto begin() const -> const_iterator;

        /** Iterate over stages in the graph in their dependency order. */
        auto end() const -> const_iterator;

    private:
        /**
         * @return None if `stage` is not present in the graph.
         */
        auto findLowestOrderIndex(RenderStage::ID stage) -> std::optional<size_t>;

        /**
         * @return bool True if the graph contains a cycle, false otherwise.
         */
        bool hasCycles() const;

        /**
         * Uses persistent information from `headStages` and `stageDeps` to
         * recalculate the iterable `orderedStages` representation.
         *
         * Must not be called if the graph may contain cycles. Check with
         * `hasCycles` first.
         */
        void recalcLayout();

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

        // Declares all dependencies. `recalcLayout` computes the `orderedStages` vector
        // from this information.
        std::unordered_map<RenderStage::ID, std::unordered_set<RenderStage::ID>> stageDeps;

        // After a call to `recalcLayout`, this is equal to `orderedStages[0]`.
        std::unordered_set<RenderStage::ID> headStages;
    };

    class RenderGraphIterator
    {
    public:
        using iterator_category = std::forward_iterator_tag;

        auto operator*() -> RenderStage::ID;
        auto operator->() -> const RenderStage::ID*;

        auto operator++() -> RenderGraphIterator&;

        bool operator==(const RenderGraphIterator& other) const;

        static auto makeBegin(const RenderGraph* graph) -> RenderGraphIterator;
        static auto makeEnd(const RenderGraph* graph) -> RenderGraphIterator;

    private:
        using OuterIter = decltype(RenderGraph::orderedStages)::const_iterator;
        using InnerIter = std::vector<RenderStage::ID>::const_iterator;

        RenderGraphIterator(const RenderGraph* graph);

        const RenderGraph* graph;
        OuterIter outer;
        InnerIter inner;
    };
} // namespace trc
