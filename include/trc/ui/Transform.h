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

    template<typename T>
    struct Vec2D
    {
        constexpr Vec2D() = default;
        constexpr Vec2D(const Vec2D& val) = default;
        constexpr Vec2D(Vec2D&& val) noexcept = default;
        ~Vec2D() noexcept = default;

        constexpr Vec2D(const T& val) : x(val), y(val) {}
        constexpr Vec2D(const T& a, const T& b) : x(a), y(b) {}

        auto operator=(const Vec2D&) -> Vec2D& = default;
        auto operator=(Vec2D&&) -> Vec2D& = default;

        // Assignment from single component
        auto operator=(const T& rhs) -> Vec2D<T>&
        {
            x = rhs;
            y = rhs;
            return *this;
        }

        constexpr auto operator<=>(const Vec2D<T>&) const = default;

        T x;
        T y;
    };

    struct Transform
    {
        struct Properties
        {
            Vec2D<Format> format{ Format::eNorm };
            Vec2D<Align> align{ Align::eRelative };
        };

        vec2 position{ 0.0f, 0.0f };
        vec2 size{ 1.0f, 1.0f };

        Properties posProp{};
        Properties sizeProp{};
    };

    class Window;

    auto concat(Transform parent, Transform child, const Window& window) noexcept -> Transform;

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
        pix_or_norm(const pix_or_norm&) = default;
        pix_or_norm(pix_or_norm&&) noexcept = default;
        pix_or_norm& operator=(const pix_or_norm&) = default;
        pix_or_norm& operator=(pix_or_norm&&) noexcept = default;
        ~pix_or_norm() noexcept = default;

        pix_or_norm(_pix v) : value(v.value), format(Format::ePixel) {}
        pix_or_norm(_norm v) : value(v.value), format(Format::eNorm) {}

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
