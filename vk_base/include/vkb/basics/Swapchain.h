#pragma once

#include <iostream>

#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "../event/Keys.h"

namespace vkb
{
    class Device;

    struct Surface
    {
        using windowDeleter = std::function<void(GLFWwindow*)>;
        using surfaceDeleter = std::function<void(vk::SurfaceKHR*)>;

        std::unique_ptr<GLFWwindow, windowDeleter> window;
        std::unique_ptr<vk::SurfaceKHR, surfaceDeleter> surface;
    };

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
     * of the Vulkan stuff as possible. Why should I use the conventional
     * terminology if Vulkan has no concept of windows?
     */
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

        /**
         * @brief Construct a swapchain
         */
        Swapchain(const Device& device, Surface s);

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
        auto getImage(uint32_t index) const noexcept -> vk::Image;

        auto acquireImage(vk::Semaphore signalSemaphore) const -> image_index;
        void presentImage(image_index image,
                          const vk::Queue& queue,
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
        void createSwapchain(bool recreate);

        const Device& device;
        std::unique_ptr<GLFWwindow, Surface::windowDeleter> window;
        std::unique_ptr<vk::SurfaceKHR, Surface::surfaceDeleter> surface;

        vk::UniqueSwapchainKHR swapchain;
        vk::Extent2D swapchainExtent;
        vk::Format swapchainFormat;

        std::vector<vk::Image> images;
        std::vector<vk::UniqueImageView> imageViews;

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

        SwapchainDependentResource(const SwapchainDependentResource&) = default;
        SwapchainDependentResource(SwapchainDependentResource&&) noexcept = default;
        auto operator=(const SwapchainDependentResource&) -> SwapchainDependentResource& = default;
        auto operator=(SwapchainDependentResource&&) noexcept -> SwapchainDependentResource& = default;
        virtual ~SwapchainDependentResource();

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
