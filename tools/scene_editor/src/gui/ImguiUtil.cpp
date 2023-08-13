#include "ImguiUtil.h"



namespace gui::util
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

void beginContextMenuStyleWindow(const char* label)
{
    // Transparent frame
    ig::PushStyleColor(ImGuiCol_TitleBg, ig::GetStyleColorVec4(ImGuiCol_WindowBg));
    ig::PushStyleColor(ImGuiCol_TitleBgActive, ig::GetStyleColorVec4(ImGuiCol_WindowBg));
    ig::PushStyleColor(ImGuiCol_FrameBg, { 0.0f, 0.0f, 0.0f, 0.0f });

    int flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize
                | ImGuiWindowFlags_AlwaysAutoResize;
    ig::Begin(label, nullptr, flags);
    ig::PopStyleColor(3);
}

} // namespace gui::util
