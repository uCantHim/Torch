#pragma once

#include <string>

#include "viewport/InputViewport.h"

class ImguiWindow : public InputViewport
{
public:
    explicit ImguiWindow(std::string windowName);

    virtual void drawWindowContent() = 0;

    void draw(trc::Frame& frame) final;

    void resize(const ViewportArea& newArea) override;
    auto getSize() -> ViewportArea override;

private:
    std::string name;
    ViewportArea viewportArea;
};
