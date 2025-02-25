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

    void draw(trc::Frame& frame) final;
    void resize(const ViewportArea& newArea) override;
    auto getSize() -> ViewportArea override;

    virtual void drawWindowContent() = 0;

    void setWindowType(ImguiWindowType type);

private:
    std::string name;

    bool wasResizedExternally{ true };
    ViewportArea viewportArea;

    ImguiWindowType windowType;
    ImGuiWindowFlags windowFlags;
};
