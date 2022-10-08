#pragma once

#include "trc/ui/elements/Quad.h"
#include "trc/ui/elements/Text.h"
#include "trc/ui/elements/BaseElements.h"

namespace trc::ui
{
    class Button : public Quad, public TextBase, public Paddable
    {
    public:
        Button() = default;
        explicit Button(std::string label);
        template<std::invocable F>
        Button(std::string label, F&& callback);
        template<std::invocable<event::Click&> F>
        Button(std::string label, F&& callback);

        void draw(DrawList& list) override;

        void setLabel(std::string newLabel);

    private:
        std::string label;
    };



    template<std::invocable F>
    Button::Button(std::string label, F&& callback)
        : Button(std::move(label), [callback](event::Click&) { callback(); })
    {
    }

    template<std::invocable<event::Click&> F>
    Button::Button(std::string label, F&& callback)
        : Button(std::move(label))
    {
        addEventListener(std::forward<F>(callback));
    }
} // namespace trc::ui
