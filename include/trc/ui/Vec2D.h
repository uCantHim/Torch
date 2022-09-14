#pragma once

namespace trc::ui
{
    /**
     * @brief Generic template for any 2-component value
     *
     * Somewhat similar to std::pair.
     */
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
} // namespace trc::ui
