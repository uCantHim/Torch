#pragma once

#include <string>
#include <vector>
#include <variant>

namespace shader_edit
{
    namespace layout
    {
        struct Location
        {
            uint location;
        };

        struct DescriptorSet
        {
            uint set;
            uint binding;

            // For example "std430" or "r32ui"
            std::vector<std::string> decorators;
        };

        struct PushConstant {};

        struct SpecializationConstant
        {
            uint constantId;
        };
    } // namespace layout

    using LayoutQualifier = std::variant<
        layout::Location,
        layout::DescriptorSet,
        layout::PushConstant,
        layout::SpecializationConstant
    >;
} // namespace shader_edit
