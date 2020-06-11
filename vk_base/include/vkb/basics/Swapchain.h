#pragma once

#include <iostream>

#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>

namespace vkb
{

class Device;
class Window;

struct Surface
{
    using windowDeleter = std::function<void(GLFWwindow*)>;
    using surfaceDeleter = std::function<void(vk::SurfaceKHR*)>;

    std::unique_ptr<GLFWwindow, windowDeleter> window;
    std::unique_ptr<vk::SurfaceKHR, surfaceDeleter> surface;
};

class SwapchainDependentResource;

class Swapchain
{
public:
    class FrameCounter
    {
        friend class Swapchain;

    public:
        static inline size_t getCurrentFrame() noexcept {
            return currentFrame;
        }

    private:
        static inline size_t currentFrame{ 0 };
    };

    using image_index = uint32_t;

    Swapchain(const Device& device, Surface s);

    auto getImageExtent() const noexcept -> vk::Extent2D;
    auto getImageFormat() const noexcept -> vk::Format;

    auto getFrameCount() const noexcept -> uint32_t;
    auto getCurrentFrame() const noexcept -> uint32_t;
    auto getImage(uint32_t index) const noexcept -> vk::Image;

    auto acquireImage(vk::Semaphore signalSemaphore) const -> image_index;
    void presentImage(image_index image,
                      const vk::Queue& queue,
                      const std::vector<vk::Semaphore>& waitSemaphores);

    auto createImageViews() const noexcept -> std::vector<vk::UniqueImageView>;

public:
    void createSwapchain(bool recreate);

    const Device& device;
    std::unique_ptr<GLFWwindow, Surface::windowDeleter> window;
    std::unique_ptr<vk::SurfaceKHR, Surface::surfaceDeleter> surface;

    vk::UniqueSwapchainKHR swapchain;
    vk::Extent2D swapchainExtent;
    vk::Format swapchainFormat;

    std::vector<vk::Image> images;

    uint32_t numFrames{ 0 };
};


class SwapchainDependentResource
{
public:
    class DependentResourceLock
    {
        friend class Swapchain;

    private:
        static void startRecreate();
        static void recreateAll(Swapchain& swapchain);
        static void endRecreate();
    };

    SwapchainDependentResource();
    ~SwapchainDependentResource();

    virtual void signalRecreateRequired() = 0;
    virtual void recreate(Swapchain&) = 0;
    virtual void signalRecreateFinished() = 0;

private:
    uint32_t index;

    static uint32_t registerSwapchainDependentResource(SwapchainDependentResource& res);
    static void removeSwapchainDependentResource(uint32_t index);

    static inline bool isRecreating{ false };
    static inline std::vector<SwapchainDependentResource*> newResources;
    static inline std::vector<SwapchainDependentResource*> dependentResources;
};

} // namespace vkb
