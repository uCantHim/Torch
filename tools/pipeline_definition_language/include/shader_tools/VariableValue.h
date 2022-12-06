#pragma once

#include <memory>
#include <string>

namespace shader_edit
{
    /**
     * @brief Render anything that is directy convertible to string
     */
    template<typename T>
        requires std::convertible_to<T, std::string>
              || std::constructible_from<std::string, T>
    inline auto render(T&& str) -> std::string
    {
        return std::string(std::forward<T>(str));
    }

    /**
     * @brief Render anything on which to_string is callable
     */
    template<typename T>
        requires requires (T a) {
            { std::to_string(a) };
        }
    inline auto render(T&& val) -> std::string
    {
        return std::to_string(val);
    }

    /**
     * @brief A type for which a render() function is defined
     */
    template<typename T>
    concept Renderable = requires (T a) {
        { render(std::forward<T>(a)) } -> std::convertible_to<std::string>;
    };

    class VariableValue
    {
    public:
        VariableValue() = default;

        template<Renderable T>
        VariableValue(T&& value) : str(render(value)) {}

        auto toString() const -> std::string {
            return str;
        }

    private:
        std::string str;
    };
} // namespace shader_edit
