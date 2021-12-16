#include "shader_edit/LayoutQualifier.h"



auto shader_edit::render(layout::Location loc) -> std::string
{
    return "layout (location = " + std::to_string(loc.location) + ")";
}

auto shader_edit::render(layout::Set set) -> std::string
{
    return "layout (set = " + std::to_string(set.set)
        + ", binding = " + std::to_string(set.binding)
        + [&] {
            std::string str;
            for (auto& dec : set.decorators) str += ", " + dec;
            return str;
        }()
        + ")";
}

auto shader_edit::render(layout::PushConstant) -> std::string
{
    return "layout (push_constant)";
}

auto shader_edit::render(layout::SpecializationConstant spec) -> std::string
{
    return "layout (constant_id = " + std::to_string(spec.constantId) + ")";
}
