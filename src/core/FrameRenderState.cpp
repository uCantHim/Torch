#include "core/FrameRenderState.h"



void trc::FrameRenderState::onRenderFinished(std::function<void()> func)
{
    std::scoped_lock lock(mutex);
    renderFinishedCallbacks.emplace_back(std::move(func));
}

void trc::FrameRenderState::signalRenderFinished()
{
    std::scoped_lock lock(mutex);
    for (auto& func : renderFinishedCallbacks) {
        func();
    }
}
