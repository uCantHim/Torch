#pragma once

#include <vector>
#include <functional>
#include <mutex>

namespace trc
{
    class FrameRenderState
    {
    public:
        void onRenderFinished(std::function<void()> func);

    private:
        friend class Renderer;
        void signalRenderFinished();

        std::mutex mutex;
        std::vector<std::function<void()>> renderFinishedCallbacks;
    };
} // namespace trc
