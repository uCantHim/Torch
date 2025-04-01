#pragma once

#include "graphics/Graphics.h"
#include "graphics/PrimitiveDraw.h"
#include "input/InputHandler.h"
#include "viewport/ViewportTree.h"

class Window;

class ViewportTreeController : public Viewport
{
public:
    ViewportTreeController(s_ptr<ViewportTree> tree,
                           Window* window,
                           GraphicsStack& graphics);

    void draw(trc::Frame& frame) override;

    void resize(const ViewportArea& newArea) override;
    auto getSize() -> ViewportArea override;

    auto notify(const UserInput& input) -> NotifyResult override;
    auto notify(const Scroll& scroll) -> NotifyResult override;
    auto notify(const CursorMovement& cursorMove) -> NotifyResult override;

    auto getInputHandler() -> InputFrame&;

private:
    Window* window;
    s_ptr<ViewportTree> tree;

    vec2 cursorPos;
    InputHandler rootHandler;

    PrimitiveRenderer primitiveRenderer;
    s_ptr<PrimitiveDrawList> drawList;
};
