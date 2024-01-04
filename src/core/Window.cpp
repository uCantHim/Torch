#include "trc/core/Window.h"

#include "trc/base/Device.h"
#include "trc/base/event/EventHandler.h"



trc::Window::Window(Instance& instance, WindowCreateInfo info)
    :
    Swapchain(
        instance.getDevice(),
        Surface(
            instance.getVulkanInstance(),
            [&] {
                info.surfaceCreateInfo.windowSize = info.size;
                info.surfaceCreateInfo.windowTitle = info.title;
                return info.surfaceCreateInfo;
            }()
        ),
        [&info]() -> s_ptr<InputProcessor> {
            if (info.inputProcessor == nullptr)
            {
                log::warn << log::here() << "Field `inputProcessor` of `WindowCreateInfo` is"
                                            " nullptr. Using `InputEventSpawner` as a default"
                                            " input processor.";
                return std::make_shared<InputEventSpawner>();
            }
            return info.inputProcessor;
        }(),
        [&]() -> SwapchainCreateInfo {
            // Always specify the storage bit
            info.swapchainCreateInfo.imageUsage |= vk::ImageUsageFlagBits::eStorage;
            return info.swapchainCreateInfo;
        }()
    ),
    instance(&instance),
    renderer(std::make_unique<Renderer>(*this))
{
    addCallbackBeforeSwapchainRecreate([this](Swapchain&) {
        renderer->waitForAllFrames();
        renderer.reset();
    });
    addCallbackAfterSwapchainRecreate([this](Swapchain&) {
        assert(renderer == nullptr);
        renderer = std::make_unique<Renderer>(*this);
    });

    setPosition(info.pos.x, info.pos.y);
}

void trc::Window::drawFrame(const vk::ArrayProxy<const DrawConfig>& draws)
{
    renderer->drawFrame(draws);
}

auto trc::Window::getInstance() -> Instance&
{
    return *instance;
}

auto trc::Window::getInstance() const -> const Instance&
{
    return *instance;
}

auto trc::Window::getDevice() -> Device&
{
    return instance->getDevice();
}

auto trc::Window::getDevice() const -> const Device&
{
    return instance->getDevice();
}

auto trc::Window::getSwapchain() -> Swapchain&
{
    return *this;
}

auto trc::Window::getSwapchain() const -> const Swapchain&
{
    return *this;
}

auto trc::Window::getRenderer() -> Renderer&
{
    return *renderer;
}
