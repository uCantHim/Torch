#pragma once

#include <vector>

#include "trc/core/FrameRenderState.h"
#include "trc/core/Task.h"

namespace trc
{
    class RenderConfig;
    class SceneBase;

    struct DrawGroup
    {
        RenderConfig* viewport;
        SceneBase* scene;

        TaskQueue taskQueue;
    };

    class Frame : public FrameRenderState
    {
    public:
        Frame(const Frame&) = delete;
        Frame(Frame&&) noexcept = delete;
        Frame& operator=(const Frame&) = delete;
        Frame& operator=(Frame&&) noexcept = delete;

        explicit Frame(const Device*);

        const Device* device;

        /**
         * @param RenderConfig& config The resources backing the viewport.
         * @param SceneBase&    scene  The scene to be drawn to the viewport.
         */
        auto addViewport(RenderConfig& config, SceneBase& scene) -> DrawGroup&;

        auto& getViewports() {
            return drawGroups;
        }

    private:
        std::vector<DrawGroup> drawGroups;
    };
} // namespace trc
