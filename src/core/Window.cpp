#include "Window.h"

#include <vkb/Device.h>



trc::Window::Window(Instance& instance, WindowCreateInfo info)
    :
    vkb::Swapchain(
        instance.getDevice(),
        vkb::makeSurface(
            instance.getVulkanInstance(),
            [&] {
                info.surfaceCreateInfo.windowSize = info.size;
                info.surfaceCreateInfo.windowTitle = info.title;
                return info.surfaceCreateInfo;
            }()
        ),
        // Swapchain create info
        [&] {
            // Always specify the storage bit
            info.swapchainCreateInfo.imageUsage |= vk::ImageUsageFlagBits::eStorage;

            // Additional flags currently only used for ray tracing
            if (instance.hasRayTracing()) {
                info.swapchainCreateInfo.imageUsage |= vk::ImageUsageFlagBits::eTransferDst;
            }
            return info.swapchainCreateInfo;
        }()
    ),
    instance(&instance),
    renderer(new Renderer(*this)),
    recreateListener(
        vkb::on<vkb::SwapchainRecreateEvent>([this](auto& e) {
            if (e.swapchain == this)
            {
                // Create a new renderer to avoid the still mysterious crash on recreate
                renderer->waitForAllFrames();
                renderer.reset(new Renderer(*this));
            }
        })
    )
{
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

auto trc::Window::getDevice() -> vkb::Device&
{
    return instance->getDevice();
}

auto trc::Window::getDevice() const -> const vkb::Device&
{
    return instance->getDevice();
}

auto trc::Window::getSwapchain() -> vkb::Swapchain&
{
    return *this;
}

auto trc::Window::getSwapchain() const -> const vkb::Swapchain&
{
    return *this;
}

auto trc::Window::getRenderer() -> Renderer&
{
    return *renderer;
}
