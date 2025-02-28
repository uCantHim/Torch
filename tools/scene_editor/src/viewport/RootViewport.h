#pragma once

#include "input/InputState.h"
#include "viewport/Viewport.h"

class App;

/**
 * @brief A wrapper that delegates to a child viewport.
 *
 * Has its own input state, which is called for any event that the child
 * viewport does not consume.
 */
class RootViewport : public Viewport
{
public:
    /**
     * @throw std::invalid_argument if `content == nullptr`.
     */
    explicit RootViewport(App& app, s_ptr<Viewport> content);

    void draw(trc::Frame& frame) override;

    void resize(const ViewportArea& newArea) override;
    auto getSize() -> ViewportArea override;

    auto notify(const UserInput& input) -> NotifyResult override;
    auto notify(const Scroll& scroll) -> NotifyResult override;
    auto notify(const CursorMovement& cursorMove) -> NotifyResult override;

    /**
     * @return The input frame that receives all events not handled by any
     *         child viewport.
     */
    auto getInputHandler() -> InputFrame&;

private:
    s_ptr<Viewport> child;
    InputStateMachine inputHandler;
};
