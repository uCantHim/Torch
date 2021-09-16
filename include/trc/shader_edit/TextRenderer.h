#pragma once

#include <string>
#include <filesystem>
namespace fs = std::filesystem;

namespace shader_edit
{
    /**
     * @brief
     */
    class ShaderCodeSource
    {
    public:
        ShaderCodeSource() = default;
        virtual ~ShaderCodeSource() = default;

        virtual auto getCode() const -> std::string = 0;
    };

    /**
     * @brief Render anything that is directy convertible to string
     */
    template<std::convertible_to<std::string> T>
    inline auto render(T&& str) -> std::string
    {
        return std::string(std::forward<T>(str));
    }

    /**
     * @brief Render anything on which to_string is callable
     */
    inline auto render(auto val) -> std::string
    {
        return std::to_string(val);
    }

    /**
     * @brief A type for which a render() function is defined
     */
    template<typename T>
    concept Renderable = requires (T a) {
        { render(a) } -> std::convertible_to<std::string>;
    };

    template<Renderable T>
    class ValueRenderer : public ShaderCodeSource
    {
    public:
        explicit ValueRenderer(T value)
            : value(std::move(value))
        {}

        auto getCode() const -> std::string override {
            return render(value);
        }

    private:
        T value;
    };
} // namespace shader_edit
