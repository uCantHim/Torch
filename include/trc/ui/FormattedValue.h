#pragma once

#include "trc/Types.h"

namespace trc::ui
{
    enum class Format : ui8
    {
        eNorm,
        ePixel,
    };

    enum class Align : ui8
    {
        eRelative,
        eAbsolute
    };

    /**
     * Internal representation of a pixel value
     */
    struct _pix
    {
        float value;
    };

    /**
     * Internal representation of a normalized value
     */
    struct _norm
    {
        float value;
    };

    /**
     * A pixel or normalized value decomposed into value and format
     */
    struct pix_or_norm
    {
        static constexpr Format globalDefaultFormat{ Format::ePixel };

        pix_or_norm(const pix_or_norm&) = default;
        pix_or_norm(pix_or_norm&&) noexcept = default;
        pix_or_norm& operator=(const pix_or_norm&) = default;
        pix_or_norm& operator=(pix_or_norm&&) noexcept = default;
        ~pix_or_norm() noexcept = default;

        pix_or_norm(_pix v) : value(v.value), format(Format::ePixel) {}
        pix_or_norm(_norm v) : value(v.value), format(Format::eNorm) {}
        pix_or_norm(float v) : value(v), format(globalDefaultFormat) {}
        pix_or_norm(float v, Format f) : value(v), format(f) {}

        float value;
        Format format;
    };

    namespace size_literals
    {
        inline constexpr _pix operator ""_px(long double val) {
            return _pix{ static_cast<float>(val) };
        }

        inline constexpr _pix operator ""_px(unsigned long long int val) {
            return _pix{ static_cast<float>(val) };
        }

        inline constexpr _norm operator ""_n(long double val) {
            return _norm{ static_cast<float>(val) };
        }

        inline constexpr _norm operator ""_n(unsigned long long int val) {
            return _norm{ static_cast<float>(val) };
        }
    } // namespace size_literals
} // namespace trc::ui
