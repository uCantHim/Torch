#pragma once

#include <string>
#include <vector>

namespace shader_edit
{
    namespace layout
    {
        struct Location
        {
            uint location;
        };

        struct Set
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

    auto render(layout::Location loc) -> std::string;
    auto render(layout::Set set) -> std::string;
    auto render(layout::PushConstant) -> std::string;
    auto render(layout::SpecializationConstant spec) -> std::string;
} // namespace shader_edit
