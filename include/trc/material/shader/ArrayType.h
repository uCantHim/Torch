#pragma once

#include <format>
#include <string>

#include "BasicType.h"

namespace trc::shader
{
    struct ArrayType
    {
        constexpr
        ArrayType(BasicType type, ui32 count)
            : type(type), count(count) {}

        auto to_string() const -> std::string {
            return std::format("{}[{}]", type.to_string(), count);
        }

        /** @return ui32 Size of the type in bytes */
        constexpr
        auto size() const -> ui32 {
            return type.size() * count;
        }

        /** @return ui32 The number of shader locations the type occupies */
        constexpr
        auto locations() const -> ui32 {
            return glm::ceil(static_cast<float>(size()) / 16.0f);
        }

        bool operator==(const ArrayType&) const = default;

        BasicType type;
        ui32 count;
    };
} // namespace trc::shader
