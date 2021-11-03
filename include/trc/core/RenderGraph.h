#pragma once

#include <vector>
#include <unordered_map>
#include <optional>
#include <functional>

#include "Types.h"
#include "RenderStage.h"
#include "RenderLayout.h"

namespace trc
{
    class Window;
    class RenderPass;

    /**
     * @brief Declares the layout of a complete render pathway
     *
     * Can be compiled into a RenderLayout structure, which can be used to
     * collect all draw commands from a scene that are relevant to the
     * layout.
     */
    class RenderGraph
    {
        friend class RenderLayout;

    public:
        void first(RenderStage::ID newStage);
        void before(RenderStage::ID nextStage, RenderStage::ID newStage);
        void after(RenderStage::ID prevStage, RenderStage::ID newStage);

        /**
         * @brief Create a stage-to-stage execution dependency
         *
         * All commands in `requiredStage` must have completed execution
         * before any command in `stage` can be executed.
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

        void addPass(RenderStage::ID stage, RenderPass& newPass);
        void removePass(RenderStage::ID stage, RenderPass& pass);

        auto compile(const Window& window) const -> RenderLayout;

    private:
        struct StageInfo
        {
            RenderStage::ID stage;
            std::vector<RenderPass*> renderPasses;

            std::vector<RenderStage::ID> waitDependencies;
        };

        using StageIterator = typename std::vector<StageInfo>::iterator;
        using StageConstIterator = typename std::vector<StageInfo>::const_iterator;

        auto findStage(RenderStage::ID stage) -> std::optional<StageIterator>;
        auto findStage(RenderStage::ID stage) const -> std::optional<StageConstIterator>;
        void insert(StageIterator next, RenderStage::ID newStage);

        std::vector<StageInfo> stages;
    };
} // namespace trc
