#pragma once

#include "trc/ui/elements/Quad.h"
#include "trc/ui/elements/Text.h"
#include "trc/ui/Window.h"

namespace trc::ui
{
    class Button : public Quad
    {
    public:
        explicit Button(Window& window, std::string label);

        template<std::invocable F>
        Button(Window& window, std::string label, F&& callback);
        template<std::invocable<event::Click&> F>
        Button(Window& window, std::string label, F&& callback);

        void setLabel(std::string newLabel);

    private:
        UniqueElement<Text> text;
    };



    template<std::invocable F>
    Button::Button(Window& window, std::string label, F&& callback)
        : Button(window, std::move(label), [callback](event::Click&) { callback(); })
    {
    }

    template<std::invocable<event::Click&> F>
    Button::Button(Window& window, std::string label, F&& callback)
        : Button(window, std::move(label))
    {
        addEventListener(std::forward<F>(callback));
    }
} // namespace trc::ui
