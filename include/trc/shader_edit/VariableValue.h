#pragma once

#include <memory>

#include "TextRenderer.h"

namespace shader_edit
{
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
