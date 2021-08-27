#include "Window.h"

#include <vkb/VulkanBase.h>



trc::Window::Window(const Instance& instance, WindowCreateInfo info)
    :
    instance(instance),
    swapchain([&] {
        // Additional flags currently only used for ray tracing
        if (instance.hasRayTracing())
        {
            info.swapchainCreateInfo.imageUsage |= vk::ImageUsageFlagBits::eTransferDst
                                                   | vk::ImageUsageFlagBits::eStorage;
        }

        return vkb::Swapchain(
            instance.getDevice(),
            vkb::createSurface(
                instance.getVulkanInstance(),
                { .windowSize={ info.size.x, info.size.y }, .windowTitle=info.title }
            ),
            info.swapchainCreateInfo
        );
    }()),
    renderer(*this)
{
}

void trc::Window::drawFrame(const DrawConfig& drawConfig)
{
    renderer.drawFrame(drawConfig);
}

auto trc::Window::getInstance() const -> const Instance&
{
    return instance;
}

auto trc::Window::getDevice() const -> const vkb::Device&
{
    return instance.getDevice();
}

auto trc::Window::getSwapchain() -> vkb::Swapchain&
{
    return swapchain;
}

auto trc::Window::getSwapchain() const -> const vkb::Swapchain&
{
    return swapchain;
}

auto trc::Window::makeFullscreenRenderArea() const -> RenderArea
{
    auto extent = swapchain.getImageExtent();
    return {
        vk::Viewport(0, 0, extent.width, extent.height, 0.0f, 1.0f),
        vk::Rect2D({ 0, 0 }, { extent.width, extent.height })
    };
}
