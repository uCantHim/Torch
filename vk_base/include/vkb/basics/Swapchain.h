#pragma once

#include <iostream>

#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "../event/Keys.h"

namespace vkb
{
    struct Surface
    {
        using windowDeleter = std::function<void(GLFWwindow*)>;
        using surfaceDeleter = std::function<void(vk::SurfaceKHR*)>;

        std::unique_ptr<GLFWwindow, windowDeleter> window;
        std::unique_ptr<vk::SurfaceKHR, surfaceDeleter> surface;
    };

    class Device;
    class SwapchainDependentResource;

    /**
     * Call this once per frame.
     *
     * Just calls glfwPollEvents().
     */
    extern void pollEvents();

    /**
     * @brief A swapchain
     *
     * This is the equivalent to the classical 'Window' class. I didn't
     * want to wrap this in a Window class because I want to expose as much
     * of the Vulkan stuff as possible. I don't see a reason to use the
     * conventional terminology if Vulkan has no concept of windows.
     */
    class Swapchain
    {
    public:
        using image_index = uint32_t;

        /**
         * @brief Construct a swapchain
         */
        Swapchain(const Device& device, Surface s);
        Swapchain(Swapchain&&) noexcept = default;
        ~Swapchain() = default;

        Swapchain(const Swapchain&) = delete;
        auto operator=(const Swapchain&) -> Swapchain& = delete;
        auto operator=(Swapchain&&) noexcept -> Swapchain& = delete;

        /**
         * @return GLFWwindow* The GLFW window handle of the swapchain's surface
         */
        auto getGlfwWindow() const noexcept -> GLFWwindow*;

        /**
         * @return vk::Extent2D Size of the swapchain images, i.e. the window size
         */
        auto getImageExtent() const noexcept -> vk::Extent2D;

        /**
         * @return vk::Format The format of the swapchain images
         */
        auto getImageFormat() const noexcept -> vk::Format;

        /**
         * @return uint32_t Number of images in the swapchain
         */
        auto getFrameCount() const noexcept -> uint32_t;

        /**
         * @return uint32_t The index of the currently active image
         */
        auto getCurrentFrame() const noexcept -> uint32_t;

        /**
         * @brief Get a specific swapchain image
         * @return vk::Image
         */
        auto getImage(uint32_t index) const noexcept -> vk::Image;

        auto acquireImage(vk::Semaphore signalSemaphore) const -> image_index;
        void presentImage(image_index image,
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
        void initGlfwCallbacks(GLFWwindow* window);
        void createSwapchain();

        const Device& device;
        std::unique_ptr<GLFWwindow, Surface::windowDeleter> window;
        std::unique_ptr<vk::SurfaceKHR, Surface::surfaceDeleter> surface;

        vk::UniqueSwapchainKHR swapchain;
        vk::Extent2D swapchainExtent;
        vk::Format swapchainFormat;

        std::vector<vk::Image> images;
        std::vector<vk::UniqueImageView> imageViews;

        uint32_t numFrames{ 0 };
        uint32_t currentFrame{ 0 };
    };
} // namespace vkb
