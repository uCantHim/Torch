#pragma once

#include "trc/ui/Element.h"

namespace trc::ui
{
    class Quad : public Element
    {
    public:
        explicit Quad(Window& window) : Element(window) {}

        void draw(DrawList& drawList) override
        {
            drawList.push(types::Quad{}, *this);
        }
    };
} // namespace trc::ui
