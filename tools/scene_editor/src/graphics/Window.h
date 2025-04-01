#pragma once

#include <trc/Torch.h>
using namespace trc::basic_types;

#include "viewport/ViewportTree.h"
#include "viewport/ViewportTreeController.h"

class Window
{
public:
    Window(const Window&) = delete;
    Window(Window&&) noexcept = delete;
    Window& operator=(const Window&) = delete;
    Window& operator=(Window&&) noexcept = delete;

    Window(GraphicsStack& graphics,
           s_ptr<trc::Window> torchWindow,
           s_ptr<Viewport> rootViewport);

    ~Window() noexcept;

    s_ptr<trc::Window> torchWindow;
    u_ptr<trc::Renderer> renderer;

    s_ptr<ViewportTree> viewportTree;
    s_ptr<ViewportTreeController> rootViewport;

    void setCursorShape(trc::CursorShape shape);

    void drawContent(trc::Frame& frame);
    void submitFrame(u_ptr<trc::Frame> frame);

private:
    std::optional<trc::CursorShape> selectedCursor;
};
