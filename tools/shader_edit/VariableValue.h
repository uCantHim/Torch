#pragma once

#include <memory>

#include "VariableValueSource.h"

namespace shader_edit
{
    /**
     * @brief
     */
    class VariableValue
    {
    public:
        VariableValue()
            : source(std::make_unique<ValueConverter<std::string>>(""))
        {}

        /**
         * @brief
         */
        template<Renderable T>
        VariableValue(T&& value)
            : source(std::make_unique<ValueConverter<T>>(std::forward<T>(value)))
        {}

        VariableValue(const VariableValue&);
        VariableValue(VariableValue&&) noexcept = default;
        auto operator=(const VariableValue&) -> VariableValue&;
        auto operator=(VariableValue&&) noexcept -> VariableValue& = default;
        ~VariableValue() = default;

        auto toString() const -> std::string;

    private:
        std::unique_ptr<VariableValueSource> source;
    };
} // namespace shader_edit
