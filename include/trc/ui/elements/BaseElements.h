#pragma once

#include "trc/Types.h"
#include "trc/ui/Style.h"
#include "trc/ui/Transform.h"

namespace trc::ui
{
    class TextBase
    {
    public:
        TextBase() = default;
        TextBase(ui32 font, ui32 size);

        void setFont(ui32 fontIndex);
        void setFontSize(ui32 fontSize);

    protected:
        ui32 fontIndex{ DefaultStyle::font };
        ui32 fontSize{ DefaultStyle::fontSize };
    };
} // namespace trc::ui
