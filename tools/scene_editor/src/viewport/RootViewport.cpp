#include "viewport/RootViewport.h"

#include <trc_util/Assert.h>



RootViewport::RootViewport(App&, s_ptr<Viewport> content)
{
    assert_arg(content != nullptr);
    child = std::move(content);
}

void RootViewport::draw(trc::Frame& frame)
{
    child->draw(frame);
}

void RootViewport::resize(const ViewportArea& newArea)
{
    child->resize(newArea);
}

auto RootViewport::getSize() -> ViewportArea
{
    return child->getSize();
}

auto RootViewport::notify(const UserInput& event) -> NotifyResult
{
    if (child->notify(event) == NotifyResult::eConsumed) {
        return NotifyResult::eConsumed;
    }
    return inputHandler.notify(event);
}

auto RootViewport::notify(const Scroll& event) -> NotifyResult
{
    if (child->notify(event) == NotifyResult::eConsumed) {
        return NotifyResult::eConsumed;
    }
    return inputHandler.notify(event);
}

auto RootViewport::notify(const CursorMovement& event) -> NotifyResult
{
    if (child->notify(event) == NotifyResult::eConsumed) {
        return NotifyResult::eConsumed;
    }
    return inputHandler.notify(event);
}

auto RootViewport::getInputHandler() -> InputFrame&
{
    return inputHandler.getRootFrame();
}
