#include "graphics/Window.h"

#include <imgui.h>
namespace ig = ImGui;

#include "input/InputProcessor.h"



/** @brief Translate the ImGuiMouseCursor enum to Torch's CursorShape. */
auto toTorchEnum(ImGuiMouseCursor cursor) -> trc::CursorShape
{
    switch (cursor)
    {
    case ImGuiMouseCursor_Arrow: return trc::CursorShape::eArrow;
    case ImGuiMouseCursor_Hand: return trc::CursorShape::ePointingHand;
    case ImGuiMouseCursor_TextInput: return trc::CursorShape::eBeam;
    case ImGuiMouseCursor_ResizeEW: return trc::CursorShape::eResizeHorizontal;
    case ImGuiMouseCursor_ResizeNS: return trc::CursorShape::eResizeVertical;
    case ImGuiMouseCursor_ResizeNESW: return trc::CursorShape::eResizeDiagonalURtoLL;
    case ImGuiMouseCursor_ResizeNWSE: return trc::CursorShape::eResizeDiagonalULtoLR;
    case ImGuiMouseCursor_ResizeAll: return trc::CursorShape::eResize;
    case ImGuiMouseCursor_NotAllowed: return trc::CursorShape::eNotAllowed;
    default:
        throw std::logic_error("Invalid ImGuiMouseCursor value.");
    }
}



Window::Window(
    GraphicsStack& graphics,
    s_ptr<trc::Window> _torchWindow,
    s_ptr<Viewport> _rootViewport)
    :
    torchWindow(_torchWindow),
    renderer(std::make_unique<trc::Renderer>(graphics.getDevice(), *torchWindow)),

    viewportTree(std::make_shared<ViewportTree>(
        ViewportArea{ { 0, 0 }, torchWindow->getSize() },
        _rootViewport
    )),
    rootViewport(std::make_shared<ViewportTreeController>(viewportTree, this, graphics))
{
    torchWindow->setInputProcessor(std::make_unique<InputProcessor>(rootViewport));
}

Window::~Window() noexcept
{
    torchWindow->setInputProcessor(std::make_unique<trc::NullInputProcessor>());
    renderer->waitForAllFrames();
}

void Window::setCursorShape(trc::CursorShape shape)
{
    selectedCursor = shape;
}

void Window::drawContent(trc::Frame& frame)
{
    rootViewport->draw(frame);
}

void Window::submitFrame(u_ptr<trc::Frame> frame)
{
    renderer->renderFrameAndPresent(std::move(frame), *torchWindow);

    // Honor requests from ImGui to set the mouse cursor shape (decided after
    // ig::End is called).
    // Overrides any cursor settings done during the frame.
    const auto imguiCursor = ig::GetMouseCursor();
    if (imguiCursor != ImGuiMouseCursor_None && imguiCursor != ImGuiMouseCursor_Arrow) {
        torchWindow->setCursorShape(toTorchEnum(imguiCursor));
    }
    else if (selectedCursor) {
        torchWindow->setCursorShape(*selectedCursor);
    }
    else {
        torchWindow->setCursorShape(trc::CursorShape::eDefault);
    }
}
