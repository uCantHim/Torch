#pragma once

#include <memory>

#include "ShaderCodeSource.h"

namespace shader_edit
{
    /**
     * @brief Render anything that is directy convertible to string
     */
    template<std::convertible_to<std::string> T>
    inline auto render(T&& str) -> std::string
    {
        return std::string(std::forward<T>(str));
    }

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

    /**
     * @brief
     */
    class VariableValue
    {
    public:
        /**
         * @brief
         */
        template<Renderable T>
        VariableValue(T&& value)
            : source(std::make_unique<ValueRenderer<T>>(std::forward<T>(value)))
        {}

        VariableValue(VariableValue&&) noexcept = default;
        auto operator=(VariableValue&&) noexcept -> VariableValue& = default;

        auto toString() const -> std::string {
            return source->getCode();
        }

    private:
        std::unique_ptr<ShaderCodeSource> source;
    };
} // namespace shader_edit
