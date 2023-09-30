#include "Font.h"

#include <optional>



u_ptr<Font> globalTextFont;
u_ptr<Font> globalMonoFont;

auto getTextFont() -> Font&
{
    if (globalTextFont) {
        return *globalTextFont;
    }

    throw std::out_of_range("[In getTextFont]: No global text font has been defined. Call"
                            " `setDefaultTextFont` before creating text elements!");
}

auto getMonoFont() -> Font&
{
    if (globalMonoFont) {
        return *globalMonoFont;
    }

    throw std::out_of_range("[In getMonoFont]: No global monospaced font has been defined. Call"
                            " `setDefaultMonoFont` before creating text elements!");
}

void setGlobalTextFont(Font font)
{
    globalTextFont = std::make_unique<Font>(std::move(font));
}

void setGlobalMonoFont(Font font)
{
    globalMonoFont = std::make_unique<Font>(std::move(font));
}

void destroyGlobalFonts()
{
    globalTextFont.reset();
    globalMonoFont.reset();
}
