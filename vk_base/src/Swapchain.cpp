#include "Swapchain.h"

#include <stdexcept>
#include <chrono>
using namespace std::chrono;

#include "PhysicalDevice.h"
#include "Device.h"
#include "VulkanDebug.h"
#include "event/EventHandler.h"
#include "event/InputEvents.h"
#include "event/WindowEvents.h"



auto vkb::makeSurface(vk::Instance instance, SurfaceCreateInfo info) -> Surface
{
    Surface result;

    // Create GLFW window
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_VISIBLE,   !info.hidden);
    glfwWindowHint(GLFW_MAXIMIZED, info.maximized);
    glfwWindowHint(GLFW_RESIZABLE, info.resizeable);
    glfwWindowHint(GLFW_FLOATING,  info.floating);
    glfwWindowHint(GLFW_DECORATED, info.decorated);
    glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, info.transparentFramebuffer);
    result.window = std::unique_ptr<GLFWwindow, Surface::WindowDeleter>(
        glfwCreateWindow(
            info.windowSize.x, info.windowSize.y,
            info.windowTitle.c_str(),
            nullptr, nullptr
        ),
        [](GLFWwindow* windowPtr) {
            glfwDestroyWindow(windowPtr);
        }
    );

    if (result.window == nullptr)
    {
        const char* errorMsg{ nullptr };
        glfwGetError(&errorMsg);
        throw std::runtime_error("[In makeSurface]: Unable to create a window: "
                                 + std::string(errorMsg));
    }
    GLFWwindow* _window = result.window.get();

    // Create Vulkan surface
    VkSurfaceKHR _surface;
    if (glfwCreateWindowSurface(instance, _window, nullptr, &_surface) != VK_SUCCESS)
    {
        const char* errorMsg{ nullptr };
        glfwGetError(&errorMsg);
        throw std::runtime_error("[In makeSurface]: Unable to create window surface: "
                                 + std::string(errorMsg));
    }
    result.surface = std::unique_ptr<vk::SurfaceKHR, Surface::SurfaceDeleter> {
        new vk::SurfaceKHR(_surface),
        [instance](vk::SurfaceKHR* oldSurface) {
            instance.destroySurfaceKHR(*oldSurface);
            delete oldSurface;
        }
    };

    return result;
}



void vkb::pollEvents()
{
    glfwPollEvents();
}



struct ImageSharingDetails {
    vk::SharingMode imageSharingMode{};
    std::vector<uint32_t> imageSharingQueueFamilies;
};

auto findOptimalImageSharingMode(const vkb::PhysicalDevice& physicalDevice) -> ImageSharingDetails
{
    ImageSharingDetails result;

    std::vector<uint32_t> imageSharingQueueFamilies;
    const auto& graphicsFamilies = physicalDevice.queueFamilyCapabilities.graphicsCapable;
    const auto& presentationFamilies = physicalDevice.queueFamilyCapabilities.presentationCapable;
    if (graphicsFamilies.empty() || presentationFamilies.empty())
    {
        throw std::runtime_error(
            "Unable to create swapchain; no graphics or presentation queues available."
            "This should have been checked during device selection."
        );
    }

    const auto& graphics = graphicsFamilies[0];
    const auto& present = presentationFamilies[0];
    if (graphics.index == present.index)
    {
        // The graphics and the presentation queues are of the same family.
        // This is preferred because exclusive access for one queue family
        // will likely be faster than concurrent access.
        result.imageSharingMode = vk::SharingMode::eExclusive;
        result.imageSharingQueueFamilies = {}; // Not important for exclusive sharing
    }
    else
    {
        // The graphics and the presentation queues are of different families.
        result.imageSharingMode = vk::SharingMode::eConcurrent;
        result.imageSharingQueueFamilies = { graphics.index, present.index };
    }

    return result;
}

auto findOptimalImageExtent(
    const vk::SurfaceCapabilitiesKHR& capabilities,
    GLFWwindow* window
    ) -> vk::Extent2D
{
    auto& extent = capabilities.currentExtent;
    if (extent.width != UINT32_MAX && extent.height != UINT32_MAX)
    {
        // The extent has already been determined by the implementation
        return extent;
    }

    int width;
    int height;
    glfwGetFramebufferSize(window, &width, &height);

    vk::Extent2D actualExtent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
    actualExtent.width = std::clamp(actualExtent.width,
                                    capabilities.minImageExtent.width,
                                    capabilities.maxImageExtent.width);
    actualExtent.height = std::clamp(actualExtent.height,
                                     capabilities.minImageExtent.height,
                                     capabilities.maxImageExtent.height);

    return actualExtent;
}

auto findOptimalSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& formats)
    -> vk::SurfaceFormatKHR
{
    if constexpr (vkb::enableVerboseLogging)
    {
        std::cout << "   Possible surface formats: ";
        for (const auto& format : formats)
        {
            std::cout << "(" << vk::to_string(format.format) << " - "
                << vk::to_string(format.colorSpace) << "), ";
        }
        std::cout << "\b\b \n";
    }

    for (const auto& format : formats)
    {
        if (format.format == vk::Format::eR8G8B8A8Unorm
            && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
        {
            if constexpr (vkb::enableVerboseLogging)
            {
                std::cout << "   Found optimal surface format \"" << vk::to_string(format.format)
                    << "\" in the color space \"" << vk::to_string(format.colorSpace) << "\"\n";
            }
            return format;
        }
    }

    if constexpr (vkb::enableVerboseLogging) {
        std::cout << "   Picked suboptimal surface format.\n";
    }
    return formats[0];
}

auto findOptimalSurfacePresentMode(const std::vector<vk::PresentModeKHR>& presentModes,
                                   vk::PresentModeKHR preferredMode = vk::PresentModeKHR::eMailbox)
    -> vk::PresentModeKHR
{
    if constexpr (vkb::enableVerboseLogging)
    {
        std::cout << "   Possible present modes: ";
        for (const auto& mode : presentModes) {
            std::cout << vk::to_string(mode) << ", ";
        }
        std::cout << "\b\b \n";
    }

    bool immediateSupported{ false };
    for (const auto& mode : presentModes)
    {
        if (mode == preferredMode)
        {
            if constexpr (vkb::enableVerboseLogging) {
                std::cout << "   Using preferred present mode: " << vk::to_string(mode) << "\n";
            }
            return mode;
        }

        if (mode == vk::PresentModeKHR::eImmediate) {
            immediateSupported = true;
        }
    }

    if constexpr (vkb::enableVerboseLogging) {
        std::cout << "   Preferred present mode is not supported\n";
    }

    if (immediateSupported) {
        return vk::PresentModeKHR::eImmediate;
    }
    else {
        return vk::PresentModeKHR::eFifo;  // The only mode that is required to be supported
    }
}



// ---------------------------- //
//        Swapchain class       //
// ---------------------------- //

vkb::Swapchain::Swapchain(const Device& device, Surface s, const SwapchainCreateInfo& info)
    :
    FrameClock([&device, &s, &info] {
        const auto& phys = device.getPhysicalDevice();
        const auto capabilities = phys.getSwapchainSupport(*s.surface).surfaceCapabilities;
        const uint32_t minImages = capabilities.minImageCount;
        const uint32_t maxImages = capabilities.maxImageCount == 0  // 0 means no limit
                                   ? std::numeric_limits<uint32_t>::max()
                                   : capabilities.maxImageCount;

        const uint32_t numFrames = glm::clamp(info.imageCount, minImages, maxImages);
        if (numFrames != info.imageCount)
        {
            throw std::runtime_error(
                "[In Swapchain::Swapchain]: Image count of " + std::to_string(info.imageCount)
                + " is not supported! The device support a minimum of " + std::to_string(minImages)
                + " and a maximum of " + std::to_string(maxImages) + "."
            );
        }

        return numFrames;
    }()),
    device(device),
    createInfo(info),
    window(std::move(s.window)),
    surface(std::move(s.surface)),
    swapchainImageUsage([&info] {
        // Add necessary default usage flags
        auto usageFlags = info.imageUsage;
        usageFlags |= vk::ImageUsageFlagBits::eColorAttachment;
        return usageFlags;
    }())
{
    /**
     * This call also has practical significance: Vulkan requires that the
     * getSurfaceSupportKHR function be called on every surface before a
     * swapchain is created on it.
     */
    if (!device.getPhysicalDevice().hasSurfaceSupport(*surface))
    {
        throw std::runtime_error(
            "In Swapchain::Swapchain(): Physical device does not have surface"
            " support for the specified surface! This means that none of the"
            " device's queue families returned true from a call to"
            " vkGetPhysicalDeviceSurfaceSupportKHR."
        );
    }

    initGlfwCallbacks(window.get());
    createSwapchain(info);
}

auto vkb::Swapchain::acquireImage(vk::Semaphore signalSemaphore) const -> uint32_t
{
    auto result = device->acquireNextImageKHR(*swapchain, UINT64_MAX, signalSemaphore, vk::Fence());
    if (result.result == vk::Result::eErrorOutOfDateKHR)
    {
        // Recreate swapchain?
        std::cout << "--- Image acquisition threw error! Investigate this since I have not"
            << " decied what to do here! (in Swapchain::acquireImage())\n";
    }
    return result.value;
}

void vkb::Swapchain::presentImage(
    uint32_t image,
    vk::Queue queue,
    const std::vector<vk::Semaphore>& waitSemaphores)
{
    vk::PresentInfoKHR presentInfo(waitSemaphores, *swapchain, image);

    try {
        auto result = queue.presentKHR(presentInfo);
        if (result == vk::Result::eSuboptimalKHR && enableVerboseLogging) {
            std::cout << "--- Swapchain has become suboptimal. Do nothing.\n";
        }
    }
    catch (const vk::OutOfDateKHRError&)
    {
        if constexpr (enableVerboseLogging) {
            std::cout << "\n--- Swapchain has become invalid, create a new one.\n";
        }
        createSwapchain(createInfo);
        return;
    }

    FrameClock::endFrame();
}

auto vkb::Swapchain::getImageExtent() const noexcept -> vk::Extent2D
{
    return swapchainExtent;
}

auto vkb::Swapchain::getImageFormat() const noexcept -> vk::Format
{
    return swapchainFormat;
}

auto vkb::Swapchain::getImageUsage() const noexcept -> vk::ImageUsageFlags
{
    return swapchainImageUsage;
}

auto vkb::Swapchain::getImage(uint32_t index) const noexcept -> vk::Image
{
    return images[index];
}

auto vkb::Swapchain::getImageView(uint32_t imageIndex) const noexcept -> vk::ImageView
{
    assert(imageIndex < getFrameCount());

    return *imageViews[imageIndex];
}

void vkb::Swapchain::setPresentMode(vk::PresentModeKHR newMode)
{
    createInfo.presentMode = newMode;
    createSwapchain(createInfo);
}



////////////////////
//  Window stuff  //
////////////////////

auto vkb::Swapchain::getGlfwWindow() const noexcept -> GLFWwindow*
{
    return window.get();
}

auto vkb::Swapchain::isOpen() const noexcept -> bool
{
    return !glfwWindowShouldClose(window.get());
}

auto vkb::Swapchain::shouldClose() const noexcept -> bool
{
    return !isOpen();
}

auto vkb::Swapchain::getSize() const noexcept -> glm::uvec2
{
    return { swapchainExtent.width, swapchainExtent.height };
}

auto vkb::Swapchain::getWindowSize() const -> glm::uvec2
{
    glm::ivec2 winSize;
    glfwGetWindowSize(window.get(), &winSize.x, &winSize.y);
    return winSize;
}

void vkb::Swapchain::resize(uint32_t width, uint32_t height)
{
    glfwSetWindowSize(window.get(), width, height);

    // I don't have to recreate here. The next present will return an
    // out-of-date swapchain.
}

void vkb::Swapchain::maximize()
{
    glfwMaximizeWindow(window.get());
}

void vkb::Swapchain::minimize()
{
    glfwIconifyWindow(window.get());
}

auto vkb::Swapchain::isMaximized() const -> bool
{
    return glfwGetWindowAttrib(window.get(), GLFW_MAXIMIZED);
}

auto vkb::Swapchain::isMinimized() const -> bool
{
    return glfwGetWindowAttrib(window.get(), GLFW_ICONIFIED);
}

void vkb::Swapchain::restore()
{
    glfwRestoreWindow(window.get());
}

auto vkb::Swapchain::getAspectRatio() const noexcept -> float
{
    return static_cast<float>(swapchainExtent.width) / static_cast<float>(swapchainExtent.height);
}

void vkb::Swapchain::forceAspectRatio(int32_t width, int32_t height)
{
    glfwSetWindowAspectRatio(window.get(), width, height);
}

void vkb::Swapchain::forceAspectRatio(const bool forced)
{
    if (forced) {
        forceAspectRatio(getSize().x, getSize().y);
    }
    else {
        forceAspectRatio(GLFW_DONT_CARE, GLFW_DONT_CARE);
    }
}

void vkb::Swapchain::hide()
{
    glfwHideWindow(window.get());
}

void vkb::Swapchain::show()
{
    glfwShowWindow(window.get());
}

void vkb::Swapchain::setUserResizeable(const bool resizeable)
{
    glfwSetWindowAttrib(window.get(), GLFW_RESIZABLE, resizeable);
}

auto vkb::Swapchain::isUserResizeable() const -> bool
{
    return glfwGetWindowAttrib(window.get(), GLFW_RESIZABLE);
}

void vkb::Swapchain::setDecorated(bool decorated)
{
    glfwSetWindowAttrib(window.get(), GLFW_DECORATED, decorated);
}

auto vkb::Swapchain::getDecorated() const -> bool
{
    return glfwGetWindowAttrib(window.get(), GLFW_DECORATED);
}

void vkb::Swapchain::setFloating(bool floating)
{
    glfwSetWindowAttrib(window.get(), GLFW_FLOATING, floating);
}

auto vkb::Swapchain::getFloating() const -> bool
{
    return glfwGetWindowAttrib(window.get(), GLFW_FLOATING);
}

void vkb::Swapchain::setOpacity(float opacity)
{
    glfwSetWindowOpacity(window.get(), opacity);
}

auto vkb::Swapchain::getOpacity() const -> float
{
    return glfwGetWindowOpacity(window.get());
}

auto vkb::Swapchain::getPosition() const -> glm::ivec2
{
    glm::ivec2 pos;
    glfwGetWindowPos(window.get(), &pos.x, &pos.y);
    return pos;
}

void vkb::Swapchain::setPosition(int32_t x, int32_t y)
{
    glfwSetWindowPos(window.get(), x, y);
}

void vkb::Swapchain::setTitle(const char* title)
{
    glfwSetWindowTitle(window.get(), title);
}

auto vkb::Swapchain::getKeyState(Key key) const -> InputAction
{
    return static_cast<InputAction>(glfwGetKey(window.get(), static_cast<int>(key)));
}

auto vkb::Swapchain::getMouseButtonState(MouseButton button) const -> InputAction
{
    return static_cast<InputAction>(glfwGetMouseButton(window.get(), static_cast<int>(button)));
}

auto vkb::Swapchain::getMousePosition() const -> glm::vec2
{
    glm::dvec2 pos;
    glfwGetCursorPos(window.get(), &pos.x, &pos.y);

    return pos;
}

auto vkb::Swapchain::getMousePositionLowerLeft() const -> glm::vec2
{
    glm::dvec2 pos;
    glfwGetCursorPos(window.get(), &pos.x, &pos.y);

    return { pos.x, static_cast<double>(getSize().y) - pos.y };
}

auto vkb::Swapchain::isPressed(Key key) const -> bool
{
    return getKeyState(key) == InputAction::press;
}

auto vkb::Swapchain::isPressed(MouseButton button) const -> bool
{
    return getMouseButtonState(button) == InputAction::press;
}



//////////////////////
//  Callback stuff  //
//////////////////////

void onChar(GLFWwindow* window, unsigned int codepoint)
{
    auto swapchain = static_cast<vkb::Swapchain*>(glfwGetWindowUserPointer(window));
    vkb::EventHandler<vkb::CharInputEvent>::notify({ swapchain, codepoint });
}

void onKey(GLFWwindow* window, int key, int, int action, int mods)
{
    auto swapchain = static_cast<vkb::Swapchain*>(glfwGetWindowUserPointer(window));
    switch(action)
    {
    case GLFW_PRESS:
        vkb::EventHandler<vkb::KeyPressEvent>::notify(
            { swapchain, static_cast<vkb::Key>(key), vkb::KeyModFlags(mods) }
        );
        break;
    case GLFW_RELEASE:
        vkb::EventHandler<vkb::KeyReleaseEvent>::notify(
            { swapchain, static_cast<vkb::Key>(key), vkb::KeyModFlags(mods) }
        );
        break;
    case GLFW_REPEAT:
        vkb::EventHandler<vkb::KeyRepeatEvent>::notify(
            { swapchain, static_cast<vkb::Key>(key), vkb::KeyModFlags(mods) }
        );
        break;
    default:
        throw std::logic_error("");
    };
}

void onMouseMove(GLFWwindow* window, double xpos, double ypos)
{
    auto swapchain = static_cast<vkb::Swapchain*>(glfwGetWindowUserPointer(window));
    vkb::EventHandler<vkb::MouseMoveEvent>::notify(
        { swapchain, static_cast<float>(xpos), static_cast<float>(ypos) }
    );
}

void onMouseClick(GLFWwindow* window, int button, int action, int mods)
{
    auto swapchain = static_cast<vkb::Swapchain*>(glfwGetWindowUserPointer(window));
    switch (action)
    {
    case GLFW_PRESS:
        vkb::EventHandler<vkb::MouseClickEvent>::notify(
            { swapchain, static_cast<vkb::MouseButton>(button), vkb::KeyModFlags(mods) }
        );
        break;
    case GLFW_RELEASE:
        vkb::EventHandler<vkb::MouseReleaseEvent>::notify(
            { swapchain, static_cast<vkb::MouseButton>(button), vkb::KeyModFlags(mods) }
        );
        break;
    default:
        throw std::logic_error("");
    }
}

void onScroll(GLFWwindow* window, double xOffset, double yOffset)
{
    auto swapchain = static_cast<vkb::Swapchain*>(glfwGetWindowUserPointer(window));
    vkb::EventHandler<vkb::ScrollEvent>::notify(
        { swapchain, static_cast<float>(xOffset), static_cast<float>(yOffset) }
    );
}

void onClose(GLFWwindow* window)
{
    auto swapchain = static_cast<vkb::Swapchain*>(glfwGetWindowUserPointer(window));
    vkb::EventHandler<vkb::SwapchainCloseEvent>::notify({ {swapchain} });
}

void vkb::Swapchain::initGlfwCallbacks(GLFWwindow* window)
{
    glfwSetWindowUserPointer(window, this);

    // Actually, I can use lambdas here, but I find this more clean now
    glfwSetCharCallback(window, onChar);
    glfwSetKeyCallback(window, onKey);
    glfwSetCursorPosCallback(window, onMouseMove);
    glfwSetMouseButtonCallback(window, onMouseClick);
    glfwSetScrollCallback(window, onScroll);

    glfwSetWindowCloseCallback(window, onClose);
}



//////////////////////////
//  Swapchain creation  //
//////////////////////////

void vkb::Swapchain::createSwapchain(const SwapchainCreateInfo& info)
{
    std::scoped_lock lock(swapchainRecreateLock);

    if constexpr (enableVerboseLogging) {
        std::cout << "\nStarting swapchain creation\n";
    }

    // Signal start of recreation
    // This allows objects depending on the swapchain to prepare the
    // recreate, like locking resources.
    EventHandler<PreSwapchainRecreateEvent>::notifySync({ {this} });

    const auto timerStart = system_clock::now();
    const auto& physDevice = device.getPhysicalDevice();

    const auto [imageSharingMode, imageSharingQueueFamilies] = findOptimalImageSharingMode(physDevice);
    const auto [capabilities, formats, presentModes] = physDevice.getSwapchainSupport(*surface);
    const auto optimalImageExtent = findOptimalImageExtent(capabilities, window.get());
    const auto optimalFormat      = findOptimalSurfaceFormat(formats);
    const auto optimalPresentMode = findOptimalSurfacePresentMode(presentModes, info.presentMode);

    // Specify the number of images in the swapchain
    const uint32_t minImages = capabilities.minImageCount;
    const uint32_t maxImages = capabilities.maxImageCount == 0  // 0 means no limit on the image maximum.
                               ? std::numeric_limits<uint32_t>::max()
                               : capabilities.maxImageCount;
    const uint32_t numFrames = glm::clamp(info.imageCount, minImages, maxImages);

    if (info.throwWhenUnsupported)
    {
        if (optimalPresentMode != info.presentMode)
        {
            throw std::runtime_error(
                "[In Swapchain::createSwapchain]: Present mode \""
                + vk::to_string(info.presentMode) + " is not supported!"
            );
        }

        if (numFrames != FrameClock::getFrameCount())
        {
            // This should not be able to happen
            throw std::runtime_error(
                "[In Swapchain::createSwapchain]: Number of images in swapchain"
                " has changed after recreation"
            );
        }
    }

    // Create the swapchain
    vk::SwapchainCreateInfoKHR createInfo(
        {},
        *surface,
        numFrames,
        optimalFormat.format,
        optimalFormat.colorSpace,
        optimalImageExtent,
        1u, // array layers
        swapchainImageUsage,
        imageSharingMode,
        static_cast<uint32_t>(imageSharingQueueFamilies.size()),
        imageSharingQueueFamilies.data(),
        capabilities.currentTransform,
        vk::CompositeAlphaFlagBitsKHR::eOpaque, // Blending with other windows
        optimalPresentMode,
        VK_TRUE, // Clipped
        *swapchain
    );
    swapchain = device->createSwapchainKHRUnique(createInfo);

    swapchainExtent = optimalImageExtent;
    swapchainFormat = optimalFormat.format;
    presentMode = optimalPresentMode;
    FrameClock::resetCurrentFrame();

    // Retrieve created images
    images = device->getSwapchainImagesKHR(*swapchain);
    imageViews.clear();
    for (size_t i = 0; i < images.size(); i++) {
        imageViews.emplace_back(createImageView(i));
    }

    if constexpr (enableVerboseLogging)
    {
        auto duration = duration_cast<milliseconds>(system_clock::now() - timerStart).count();

        std::cout << "Swapchain created (" << duration << " ms):\n";

        std::cout << "   Size: (" << swapchainExtent.width << ", " << swapchainExtent.height << ")\n";
        std::cout << "   Images: " << numFrames << "\n";
        std::cout << "   Format: " << vk::to_string(optimalFormat.format)
            << ", Color Space: " << vk::to_string(optimalFormat.colorSpace) << "\n";
        std::cout << "   Image usage: " << vk::to_string(createInfo.imageUsage) << "\n";
        std::cout << "   Image sharing mode: " << vk::to_string(imageSharingMode) << "\n";
        std::cout << "   Present mode: " << vk::to_string(optimalPresentMode) << "\n";

        std::cout << "\nRecreating swapchain-dependent resources...\n";
    }

    // Signal that recreation is finished.
    // Objects depending on the swapchain should now recreate their
    // resources.
    EventHandler<SwapchainRecreateEvent>::notifySync({ {this} });

    if constexpr (enableVerboseLogging) {
        std::cout << "Swapchain-dependent resource creation completed.\n";
    }
}

auto vkb::Swapchain::createImageView(uint32_t imageIndex) const -> vk::UniqueImageView
{
    assert(imageIndex < getFrameCount());

    return device->createImageViewUnique(
        vk::ImageViewCreateInfo(
            {}, images[imageIndex], vk::ImageViewType::e2D,
            getImageFormat(), vk::ComponentMapping(),
            { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 }
        )
    );
}
