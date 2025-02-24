#pragma once

#include <string>

#include <imgui.h>

#include "viewport/InputViewport.h"

enum class ImguiWindowType
{
    eViewport,
    eFloating,
};

class ImguiWindow : public InputViewport
{
public:
    explicit ImguiWindow(std::string windowName,
                         ImguiWindowType type = ImguiWindowType::eViewport);

    virtual void drawWindowContent() = 0;

    void setWindowType(ImguiWindowType type);

    void draw(trc::Frame& frame) final;
    void resize(const ViewportArea& newArea) override;
    auto getSize() -> ViewportArea override;

private:
    std::string name;
    ViewportArea viewportArea;

    ImguiWindowType windowType;
    ImGuiWindowFlags windowFlags;
};
