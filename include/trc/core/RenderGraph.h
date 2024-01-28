#pragma once

#include <optional>
#include <vector>

#include "trc/Types.h"
#include "trc/core/RenderStage.h"

namespace trc
{
    class Window;

    /**
     * @brief Declares the layout of a complete render pathway
     *
     * Can be compiled into a RenderLayout structure, which can be used to
     * collect all draw commands from a scene that are relevant to the
     * layout.
     */
    class RenderGraph
    {
    public:
        struct StageInfo
        {
            RenderStage::ID stage;
            std::vector<RenderStage::ID> waitDependencies;
        };

        RenderGraph() = default;
        RenderGraph(const RenderGraph&) = default;
        RenderGraph(RenderGraph&&) noexcept = default;
        RenderGraph& operator=(const RenderGraph&) = default;
        RenderGraph& operator=(RenderGraph&&) noexcept = default;
        ~RenderGraph() noexcept = default;

        void first(RenderStage::ID newStage);
        void last(RenderStage::ID newStage);

        /**
         * @throw std::out_of_range if `nextStage` is not present in the graph.
         */
        void before(RenderStage::ID nextStage, RenderStage::ID newStage);

        /**
         * @throw std::out_of_range if `prevStage` is not present in the graph.
         */
        void after(RenderStage::ID prevStage, RenderStage::ID newStage);

        /**
         * @brief Create a stage-to-stage execution dependency
         *
         * All commands in `requiredStage` must have completed execution
         * before any command in `stage` can be executed.
         *
         * TODO: This is currently not functional.
         *
         * @param RenderStage::ID stage
         * @param RenderStage::ID requiredStage
         */
        void require(RenderStage::ID stage, RenderStage::ID requiredStage);

        /**
         * @param RenderStage::ID stage
         *
         * @return bool True if the graph contains the specified stage type
         *              at least once.
         */
        bool contains(RenderStage::ID stage) const noexcept;

        /**
         * @return size_t The number of stages in the graph
         */
        auto size() const noexcept -> size_t;

        // TODO
        auto getStages() const -> const std::vector<StageInfo>& {
            return stages;
        }

    private:
        using StageIterator = typename std::vector<StageInfo>::iterator;
        using StageConstIterator = typename std::vector<StageInfo>::const_iterator;

        auto findStage(RenderStage::ID stage) -> std::optional<StageIterator>;
        auto findStage(RenderStage::ID stage) const -> std::optional<StageConstIterator>;
        void insert(StageIterator next, RenderStage::ID newStage);

        std::vector<StageInfo> stages;
    };
} // namespace trc
