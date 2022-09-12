#pragma once

#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

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
        glm::ivec2 windowSize{ 1920, 1080 };
        std::string windowTitle;

        bool hidden{ false };
        bool maximized{ false };
        bool resizeable{ true };
        bool floating{ false };
        bool decorated{ true };
        bool transparentFramebuffer{ false };
    };

    struct Surface
    {
        using WindowDeleter = std::function<void(GLFWwindow*)>;
        using SurfaceDeleter = std::function<void(vk::SurfaceKHR*)>;

        std::unique_ptr<GLFWwindow, WindowDeleter> window;
        std::unique_ptr<vk::SurfaceKHR, SurfaceDeleter> surface;
    };

    /**
     * @throw std::runtime_error if window- or surface creation fails
     */
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
        Swapchain(const Swapchain&) = delete;
        Swapchain(Swapchain&&) noexcept = delete;
        auto operator=(const Swapchain&) -> Swapchain& = delete;
        auto operator=(Swapchain&&) noexcept -> Swapchain& = delete;

        /**
         * @brief Construct a swapchain
         */
        Swapchain(const Device& device, Surface s, const SwapchainCreateInfo& info = {});
        ~Swapchain() = default;


        /////////////////////////////////////////
        //                                     //
        //   Swapchain-related functionality   //
        //                                     //
        /////////////////////////////////////////

        /**
         * @brief Acquire and image from the engine
         */
        auto acquireImage(vk::Semaphore signalSemaphore) const -> uint32_t;

        /**
         * @brief Present a previously acquired image to the engine
         *
         * @return bool False if the swapchain has been recreated, true
         *              if no special case occured.
         */
        [[nodiscard]]
        bool presentImage(uint32_t image,
                          vk::Queue queue,
                          const std::vector<vk::Semaphore>& waitSemaphores);

        /**
         * Query the **swapchain image size** in pixels via getSize or
         * getImageExtent and query the window size in screen coordinates
         * via getWindowSize.
         *
         * @return vk::Extent2D Size of the swapchain images. Same as getSize.
         */
        auto getImageExtent() const noexcept -> vk::Extent2D;

        /**
         * @return vk::Format The format of the swapchain images
         */
        auto getImageFormat() const noexcept -> vk::Format;

        /**
         * @return vk::ImageUsageFlags
         */
        auto getImageUsage() const noexcept -> vk::ImageUsageFlags;

        /**
         * @brief Get a specific swapchain image
         * @return vk::Image
         */
        auto getImage(uint32_t index) const noexcept -> vk::Image;

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
         * @brief Set a presentation mode to be used for the swapchain
         *
         * Creates a new swapchain with the desired present mode. Another
         * present mode will be used if the preferred one is not supported
         * and `throwWhenUnsupported` has not been set in the
         * SwapchainCreateInfo.
         *
         * @param vk::PresentModeKHR newMode
         *
         * @throw std::runtime_error if the new mode is not available and
         *                           `throwWhenUnsupported` has been set in
         *                           the SwapchainCreateInfo.
         */
        void setPresentMode(vk::PresentModeKHR newMode);



        //////////////////////////////////////
        //                                  //
        //   Window-related functionality   //
        //                                  //
        //////////////////////////////////////

        /**
         * @return GLFWwindow* The GLFW window handle of the swapchain's surface
         */
        auto getGlfwWindow() const noexcept -> GLFWwindow*;

        /**
         * Is the same as `!shouldClose()`.
         *
         * @return bool false if the window has been closed
         */
        auto isOpen() const noexcept -> bool;

        /**
         * Is the same as `!isOpen()`.
         *
         * @return bool true if the window has been closed
         */
        auto shouldClose() const noexcept -> bool;

        /**
         * Query the **swapchain image size** in pixels via getSize or
         * getImageExtent and query the window size in screen coordinates
         * via getWindowSize.
         *
         * @return uvec2 Size of the swapchain images in pixels
         */
        auto getSize() const noexcept -> glm::uvec2;

        /**
         * This returns the window's size in screen coordinates, which
         * might differ from the actual swapchain image size (which is in
         * pixels).
         *
         * Query the **swapchain image size** in pixels via getSize or
         * getImageExtent and query the window size in screen coordinates
         * via getWindowSize.
         *
         * @return uvec2 Window size in screen coordinates
         */
        auto getWindowSize() const -> glm::uvec2;

        /**
         * @brief Resize the swapchain
         *
         * Create a new swapchain with the specified size.
         *
         * The specified width and height are in screen coordinates, which
         * might differ from the actual resulting framebuffer size, i.e.
         * the swapchain image size.
         *
         * Query the **swapchain image size** in pixels via getSize or
         * getImageExtent and query the window size in screen coordinates
         * via getWindowSize.
         */
        void resize(uint32_t width, uint32_t height);

        void maximize();
        void minimize();
        auto isMaximized() const -> bool;
        auto isMinimized() const -> bool;

        /**
         * @brief Restore the window from maximization or minimization
         */
        void restore();

        /**
         * @return float Aspect ratio of the swapchain images (width/height)
         */
        auto getAspectRatio() const noexcept -> float;

        /**
         * @brief Force the window size to a specific aspect ratio
         *
         * Sets a forced aspect ratio of `(width / height)`.
         *
         * To disable forced aspect ratio, call `forceAspectRatio(false)`.
         *
         * @param float width
         * @param float height
         */
        void forceAspectRatio(int32_t width, int32_t height);

        /**
         * @brief Control whether the window is forced to an aspect ratio
         *
         * @param bool forced If false, disable forced aspect ratio. If
         *                    true, set the forced aspect ratio to the
         *                    window's current aspect ratio.
         */
        void forceAspectRatio(bool forced);

        void hide();
        void show();

        /**
         * @brief Set whether the window should be resizeable by the user
         *
         * It can always be resized with the `resize` function.
         */
        void setUserResizeable(bool resizeable);

        /**
         * @brief Check whether the window is resizeable by the user
         */
        auto isUserResizeable() const -> bool;

        void setDecorated(bool decorated);
        auto getDecorated() const -> bool;
        void setFloating(bool floating);
        auto getFloating() const -> bool;

        void setOpacity(float opacity);
        auto getOpacity() const -> float;

        /**
         * @brief Get the window's position on the screen
         *
         * @return ivec2 The position of the window's upper-left corner in
         *               screen coordinates.
         */
        auto getPosition() const -> glm::ivec2;

        /**
         * @brief Move the window to a position on the screen
         *
         * Move the the window's upper left corner to a new position.
         */
        void setPosition(int32_t x, int32_t y);

        /**
         * @brief Set a window title
         */
        void setTitle(const char* title);

        auto getKeyState(Key key) const -> InputAction;
        auto getMouseButtonState(MouseButton button) const -> InputAction;

        /**
         * @return vec2 Cursor position relative to the upper-left corner
         *              of the window's content area
         */
        auto getMousePosition() const -> glm::vec2;

        /**
         * @return vec2 Cursor position relative to the lower-left corner
         *              of the window's content area
         */
        auto getMousePositionLowerLeft() const -> glm::vec2;

        auto isPressed(Key key) const -> bool;
        auto isPressed(MouseButton button) const -> bool;

    public:
        const Device& device;

    private:
        void initGlfwCallbacks(GLFWwindow* window);
        void createSwapchain(const SwapchainCreateInfo& info);

        /**
         * @brief Create an image view for one of the images in the swapchain
         */
        auto createImageView(uint32_t imageIndex) const -> vk::UniqueImageView;

        std::mutex swapchainRecreateLock;

        SwapchainCreateInfo createInfo;

        std::unique_ptr<GLFWwindow, Surface::WindowDeleter> window;
        std::unique_ptr<vk::SurfaceKHR, Surface::SurfaceDeleter> surface;

        vk::UniqueSwapchainKHR swapchain;
        vk::Extent2D swapchainExtent;
        vk::Format swapchainFormat;
        vk::ImageUsageFlags swapchainImageUsage;
        vk::PresentModeKHR presentMode;

        std::vector<vk::Image> images;
        std::vector<vk::UniqueImageView> imageViews;
    };
} // namespace vkb
