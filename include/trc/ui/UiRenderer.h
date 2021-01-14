#pragma once

namespace trc::ui
{
    class Window;

    /**
     * Implement renderer as exchangeable interface because why not
     */
    class Renderer
    {
    public:
        virtual void draw(Window& window) = 0;
    };
} // namespace trc::ui
