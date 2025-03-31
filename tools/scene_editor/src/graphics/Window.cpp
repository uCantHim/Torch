#include "graphics/Window.h"

#include "input/InputProcessor.h"



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
    rootViewport(std::make_shared<ViewportTreeController>(viewportTree, torchWindow.get(), graphics))
{
    torchWindow->setInputProcessor(std::make_unique<InputProcessor>(rootViewport));
}

Window::~Window() noexcept
{
    torchWindow->setInputProcessor(std::make_unique<trc::NullInputProcessor>());
    renderer->waitForAllFrames();
}

void Window::drawFrame(u_ptr<trc::Frame> frame)
{
    rootViewport->draw(*frame);
    renderer->renderFrameAndPresent(std::move(frame), *torchWindow);
}
