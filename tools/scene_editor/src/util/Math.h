#include <concepts>
#include <limits>

#include <glm/glm.hpp>

namespace util
{
    namespace detail
    {
        template<std::floating_point T>
        constexpr T sqrtNewtonRaphson(T x, T curr, T prev)
        {
            return curr == prev
                ? curr
                : sqrtNewtonRaphson(x, T{0.5} * (curr + x / curr), curr);
        }
    }

    /**
     * @brief A constexpr sqrt
     *
     * From https://stackoverflow.com/questions/8622256/in-c11-is-sqrt-defined-as-constexpr.
     *
     * @return An approximation of `sqrt(x)` for non-negative `x`, otherwise NaN.
     */
    template<std::floating_point T>
    constexpr T sqrt(T x)
    {
        return x >= 0 && x < std::numeric_limits<T>::infinity()
            ? detail::sqrtNewtonRaphson(x, x, T{0})
            : std::numeric_limits<T>::quiet_NaN();
    }

    /**
     * @brief Calculate the squared distance between two objects.
     *
     * A constexpr version of `glm::distance2`.
     *
     * @return The squared distance between `a` and `b`.
     */
    template<glm::length_t N, std::floating_point T, glm::qualifier Q>
    constexpr auto distance2(const glm::vec<N, T, Q>& a, const glm::vec<N, T, Q>& b) -> T
    {
        static_assert(1 < N && N <= 4);
        return glm::dot(a - b, a - b);
    }
} // namespace util
