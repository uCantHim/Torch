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

    /**
     * Padding is always applied on both opposite sides of the element.
     * Padding on the x-axis is applied left and right, padding on the
     * y-axis is applied top and bottom.
     *
     * Padding formats of Format::eNorm apply padding relative to the
     * padded object's size. Example: A padding of 0.5_n on a button with a
     * width of 20 pixels will add 10 pixels to each of the button's sides.
     */
    class Paddable
    {
    public:
        Paddable() = default;
        Paddable(float x, float y, Vec2D<Format> format);

        void setPadding(vec2 pad, Vec2D<Format> format);
        void setPadding(float x, float y, Vec2D<Format> format);
        void setPadding(pix_or_norm x, pix_or_norm y);
        void setPadding(Vec2D<pix_or_norm> v);

        auto getPadding() const -> vec2;
        auto getPaddingFormat() const -> Vec2D<Format>;

    protected:
        /**
         * @param vec2 elemSize The size that the padding will be applied to
         * @param const Window& window
         *
         * @return vec2 Normalized padding value. elemSize * padding if
         *              padding format is norm.
         */
        auto getNormPadding(vec2 elemSize, const Window& window) const -> vec2;

    private:
        vec2 padding{ DefaultStyle::padding };
        Vec2D<Format> format{ DefaultStyle::paddingFormat };
    };
} // namespace trc::ui
