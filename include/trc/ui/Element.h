#pragma once

#include <vector>

#include "Types.h"
#include "Transform.h"
#include "CRTPNode.h"
#include "DrawInfo.h"

namespace trc::ui
{
    class Element : public CRTPNode<Element>, public Drawable
    {
    public:
        // void draw(std::vector<DrawInfo>& drawList, SizeVal globalPos, SizeVal globalSize) override = 0;
    };

    template<typename T>
    concept GuiElement = requires {
        std::is_base_of_v<Element, T>;
    };
} // namespace trc::ui
