#pragma once

#include <string>
#include <filesystem>
namespace fs = std::filesystem;

namespace shader_edit
{
    /**
     * @brief
     */
    class VariableValueSource
    {
    public:
        VariableValueSource() = default;
        virtual ~VariableValueSource() = default;

        virtual auto copy() const -> std::unique_ptr<VariableValueSource> = 0;

        virtual auto getCode() const -> std::string = 0;
    };

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

    template<Renderable T>
        requires std::copy_constructible<T>
    class ValueConverter : public VariableValueSource
    {
    public:
        explicit ValueConverter(T value)
            : value(std::move(value))
        {}

        auto copy() const -> std::unique_ptr<VariableValueSource> override {
            return std::make_unique<ValueConverter<T>>(value);
        }

        auto getCode() const -> std::string override {
            return render(value);
        }

    private:
        T value;
    };
} // namespace shader_edit
