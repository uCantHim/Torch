#pragma once

#include <string>

#include "text/Font.h"
#include "ui/Element.h"

namespace trc::ui
{
    class Text
    {
        Text(std::string str, Font& font);
    };
} // namespace trc::ui
