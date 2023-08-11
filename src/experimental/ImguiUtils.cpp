#include "trc/experimental/ImguiUtils.h"

namespace ig = ImGui;



namespace trc::experimental::imgui
{

WindowGuard::~WindowGuard() noexcept
{
    if (!closed) {
        ig::End();
    }
}

void WindowGuard::close()
{
    ig::End();
    closed = true;
}



StyleVar::StyleVar(ImGuiStyleVar var, const float value)
    :
    numStyleVars(1)
{
    ig::PushStyleVar(var, value);
}

StyleVar::StyleVar(std::initializer_list<std::pair<ImGuiStyleVar, float>> vars)
    :
    numStyleVars(vars.size())
{
    for (auto [var, value] : vars) {
        ig::PushStyleVar(var, value);
    }
}

StyleVar::~StyleVar() noexcept
{
    ig::PopStyleVar(numStyleVars);
    numStyleVars = 0;
}

} // namespace trc::experimental::imgui
