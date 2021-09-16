#pragma once

#include <memory>

#include "TextRenderer.h"
#include "FileInclude.h"

namespace shader_edit
{
    /**
     * @brief
     */
    class VariableValue
    {
    public:
        VariableValue()
            : source(std::make_unique<ValueRenderer<std::string>>(""))
        {}

        /**
         * @brief
         */
        template<Renderable T>
        VariableValue(T&& value)
            : source(std::make_unique<ValueRenderer<T>>(std::forward<T>(value)))
        {}

        VariableValue(const VariableValue&);
        VariableValue(VariableValue&&) noexcept = default;
        auto operator=(const VariableValue&) -> VariableValue&;
        auto operator=(VariableValue&&) noexcept -> VariableValue& = default;
        ~VariableValue() = default;

        auto toString() const -> std::string;

    private:
        std::unique_ptr<ShaderCodeSource> source;
    };
} // namespace shader_edit
