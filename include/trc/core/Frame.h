#pragma once

#include <vector>

#include "trc/core/FrameRenderState.h"
#include "trc/core/Task.h"

namespace trc
{
    class ViewportConfig;
    class SceneBase;

    struct DrawGroup
    {
        ResourceStorage* resources;
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

        Frame() = default;
        ~Frame() noexcept = default;

        /**
         * @param RenderConfig& config The resources backing the viewport.
         * @param SceneBase&    scene  The scene to be drawn to the viewport.
         */
        auto addViewport(ViewportConfig& config, SceneBase& scene) -> DrawGroup&;

        auto& getViewports() {
            return drawGroups;
        }

    private:
        std::vector<DrawGroup> drawGroups;
    };
} // namespace trc
