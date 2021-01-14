#pragma once

#include "UiRenderer.h"

namespace trc::ui
{
    class TorchRenderer : public Renderer
    {
        void draw(Window& window) override;
    };
} // namespace trc::ui
