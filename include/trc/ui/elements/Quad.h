#pragma once

#include "trc/ui/Element.h"

namespace trc::ui
{
    class Quad : public Element
    {
    protected:
        void draw(DrawList& drawList) override
        {
            drawList.push(types::Quad{}, *this);
        }
    };
} // namespace trc::ui
