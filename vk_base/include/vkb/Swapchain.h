#pragma once

#include <mutex>
#include <iostream>

#include "VulkanInclude.h"
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "event/Keys.h"
#include "FrameClock.h"

namespace vkb
{
    class Device;

    struct SurfaceCreateInfo
    {
        vk::Extent2D windowSize{ 1920, 1080 };
        std::string windowTitle;
    };

    struct Surface
    {
        using WindowDeleter = std::function<void(GLFWwindow*)>;
        using SurfaceDeleter = std::function<void(vk::SurfaceKHR*)>;

        std::unique_ptr<GLFWwindow, WindowDeleter> window;
        std::unique_ptr<vk::SurfaceKHR, SurfaceDeleter> surface;
    };

    auto makeSurface(vk::Instance instance, SurfaceCreateInfo createInfo) -> Surface;

    /**
     * Call this once per frame.
     *
     * Just calls glfwPollEvents().
     */
    void pollEvents();

    /**
     * These settings are *preferences*, the implementation is not
     * guaranteed to support all possible values.
     */
    struct SwapchainCreateInfo
    {
        /** Only eFifo is guaranteed to be supported. */
        vk::PresentModeKHR presentMode{ vk::PresentModeKHR::eMailbox };

        /**
         * A value of 0 means that the swapchain will be created with the
         * least possible number of images.
         */
        uint32_t imageCount{ 3 };

        /** The swapchain will always enable the eColorAttachment bit. */
        vk::ImageUsageFlags imageUsage;

        /**
         * If set to true, swapchain creation will throw an exception
         * (std::runtime_error) if any of the preferences are not supported
         * by the implementation.
         *
         * If false, another supported value will be chosen if the preference
         * is not supported.
         */
        bool throwWhenUnsupported{ false };
    };

    /**
     * @brief A swapchain
     *
     * This is the equivalent to the classical 'Window' class. I didn't
     * want to wrap this in a Window class because I want to expose as much
     * of the Vulkan stuff as possible. I don't see a reason to use the
     * conventional terminology if Vulkan has no concept of windows.
     */
    class Swapchain : public FrameClock
    {
    public:
        /**
         * @brief Construct a swapchain
         */
        Swapchain(const Device& device, Surface s, const SwapchainCreateInfo& info = {});
        Swapchain(Swapchain&&) noexcept = default;
        ~Swapchain() = default;

        Swapchain(const Swapchain&) = delete;
        auto operator=(const Swapchain&) -> Swapchain& = delete;
        auto operator=(Swapchain&&) noexcept -> Swapchain& = delete;

        auto isOpen() const noexcept -> bool;

        /**
         * @return GLFWwindow* The GLFW window handle of the swapchain's surface
         */
        auto getGlfwWindow() const noexcept -> GLFWwindow*;

        /**
         * @return uvec2 Size of the swapchain images
         */
        auto getSize() const noexcept -> glm::uvec2;

        /**
         * @return vk::Extent2D Size of the swapchain images, i.e. the window size
         */
        auto getImageExtent() const noexcept -> vk::Extent2D;

        /**
         * @return float Aspect ratio of the swapchain images (width/height)
         */
        auto getAspectRatio() const noexcept -> float;

        /**
         * @return vk::Format The format of the swapchain images
         */
        auto getImageFormat() const noexcept -> vk::Format;

        /**
         * @brief Get a specific swapchain image
         * @return vk::Image
         */
        auto getImage(uint32_t index) const noexcept -> vk::Image;

        /**
         * @brief Set a presentation mode to be used for the swapchain
         *
         * Another present mode will be used if the preferred one is not
         * supported.
         *
         * Recreates the swapchain.
         *
         * @param vk::PresentModeKHR newMode
         */
        void setPreferredPresentMode(vk::PresentModeKHR newMode);

        auto acquireImage(vk::Semaphore signalSemaphore) const -> uint32_t;
        void presentImage(uint32_t image,
                          vk::Queue queue,
                          const std::vector<vk::Semaphore>& waitSemaphores);

        /**
         * @brief Get an image view on one of the swapchain images
         *
         * The swapchain holds one default image view for each image in the
         * swapchain. This function queries one of these views.
         *
         * The returned view must not be destroyed manually.
         *
         * @return vk::ImageView
         */
        auto getImageView(uint32_t imageIndex) const noexcept -> vk::ImageView;

        /**
         * @brief Create an image view for one of the images in the swapchain
         */
        auto createImageView(uint32_t imageIndex) const -> vk::UniqueImageView;

        /**
         * @brief Create image views for all images in the swapchain
         *
         * Returned images are in the array position corresponding to their
         * index.
         */
        auto createImageViews() const -> std::vector<vk::UniqueImageView>;

        auto getKeyState(Key key) const -> InputAction;
        auto getMouseButtonState(MouseButton button) const -> InputAction;
        auto getMousePosition() const -> glm::vec2;

    public:
        const Device& device;

    private:
        void initGlfwCallbacks(GLFWwindow* window);
        void createSwapchain(const SwapchainCreateInfo& info);

        std::mutex swapchainRecreateLock;

        SwapchainCreateInfo createInfo;

        std::unique_ptr<GLFWwindow, Surface::WindowDeleter> window;
        std::unique_ptr<vk::SurfaceKHR, Surface::SurfaceDeleter> surface;
        bool isWindowOpen{ true };

        vk::UniqueSwapchainKHR swapchain;
        vk::Extent2D swapchainExtent;
        vk::Format swapchainFormat;
        vk::PresentModeKHR presentMode;

        std::vector<vk::Image> images;
        std::vector<vk::UniqueImageView> imageViews;
    };
} // namespace vkb
