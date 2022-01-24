#include "ImguiUtil.h"



void gui::util::beginContextMenuStyleWindow(const char* label)
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
