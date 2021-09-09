#include "basics/Swapchain.h"

#include <stdexcept>
#include <chrono>
using namespace std::chrono;

#include "basics/PhysicalDevice.h"
#include "basics/Device.h"
#include "basics/VulkanDebug.h"
#include "event/EventHandler.h"
#include "event/InputEvents.h"
#include "event/WindowEvents.h"



auto vkb::createSurface(vk::Instance instance, SurfaceCreateInfo createInfo) -> Surface
{
    Surface result;

    // Create GLFW window
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    result.window = std::unique_ptr<GLFWwindow, Surface::WindowDeleter>(
        glfwCreateWindow(
            createInfo.windowSize.width, createInfo.windowSize.height,
            createInfo.windowTitle.c_str(),
            nullptr, nullptr
        ),
        [](GLFWwindow* windowPtr) {
            glfwDestroyWindow(windowPtr);
        }
    );

    // Create Vulkan surface
    GLFWwindow* _window = result.window.get();
    VkSurfaceKHR _surface;
    if (glfwCreateWindowSurface(instance, _window, nullptr, &_surface) != VK_SUCCESS) {
        throw std::runtime_error("Unable to create window surface!");
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
    device(device),
    createInfo(info),
    window(std::move(s.window)),
    surface(std::move(s.surface))
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

auto vkb::Swapchain::isOpen() const noexcept -> bool
{
    return isWindowOpen;
}

auto vkb::Swapchain::getGlfwWindow() const noexcept -> GLFWwindow*
{
    return window.get();
}

auto vkb::Swapchain::getSize() const noexcept -> glm::uvec2
{
    return { swapchainExtent.width, swapchainExtent.height };
}

auto vkb::Swapchain::getImageExtent() const noexcept -> vk::Extent2D
{
    return swapchainExtent;
}

auto vkb::Swapchain::getAspectRatio() const noexcept -> float
{
    return static_cast<float>(swapchainExtent.width) / static_cast<float>(swapchainExtent.height);
}

auto vkb::Swapchain::getImageFormat() const noexcept -> vk::Format
{
    return swapchainFormat;
}

auto vkb::Swapchain::getFrameCount() const noexcept -> uint32_t
{
    return static_cast<uint32_t>(numFrames);
}


auto vkb::Swapchain::getCurrentFrame() const noexcept -> uint32_t
{
    return currentFrame;
}

auto vkb::Swapchain::getImage(uint32_t index) const noexcept -> vk::Image
{
    return images[index];
}

void vkb::Swapchain::setPreferredPresentMode(vk::PresentModeKHR newMode)
{
    createInfo.presentMode = newMode;
    createSwapchain(createInfo);
}

auto vkb::Swapchain::acquireImage(vk::Semaphore signalSemaphore) const -> image_index
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
    image_index image,
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

    currentFrame = (currentFrame + 1) % numFrames;
}

auto vkb::Swapchain::getImageView(uint32_t imageIndex) const noexcept -> vk::ImageView
{
    assert(imageIndex < getFrameCount());

    return *imageViews[imageIndex];
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

auto vkb::Swapchain::createImageViews() const -> std::vector<vk::UniqueImageView>
{
    std::vector<vk::UniqueImageView> result;
    result.reserve(images.size());

    for (uint32_t i = 0; i < getFrameCount(); i++)
    {
        result.push_back(createImageView(i));
    }

    return result;
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
    double x, y;
    glfwGetCursorPos(window.get(), &x, &y);

    return { static_cast<float>(x), static_cast<float>(y) };
}

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
            { swapchain, static_cast<vkb::Key>(key), mods }
        );
        break;
    case GLFW_RELEASE:
        vkb::EventHandler<vkb::KeyReleaseEvent>::notify(
            { swapchain, static_cast<vkb::Key>(key), mods }
        );
        break;
    case GLFW_REPEAT:
        vkb::EventHandler<vkb::KeyRepeatEvent>::notify(
            { swapchain, static_cast<vkb::Key>(key), mods }
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
            { swapchain, static_cast<vkb::MouseButton>(button), mods }
        );
        break;
    case GLFW_RELEASE:
        vkb::EventHandler<vkb::MouseReleaseEvent>::notify(
            { swapchain, static_cast<vkb::MouseButton>(button), mods }
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

void vkb::Swapchain::initGlfwCallbacks(GLFWwindow* window)
{
    glfwSetWindowUserPointer(window, this);

    // Actually, I can use lambdas here, but I find this more clean now
    glfwSetCharCallback(window, onChar);
    glfwSetKeyCallback(window, onKey);
    glfwSetCursorPosCallback(window, onMouseMove);
    glfwSetMouseButtonCallback(window, onMouseClick);
    glfwSetScrollCallback(window, onScroll);

    // Use a lambda here so I have access to the private member `isWindowOpen`
    glfwSetWindowCloseCallback(window, [](GLFWwindow* window) {
        auto swapchain = static_cast<vkb::Swapchain*>(glfwGetWindowUserPointer(window));
        swapchain->isWindowOpen = false;
        vkb::EventHandler<vkb::SwapchainCloseEvent>::notify({ {swapchain} });
    });
}

void vkb::Swapchain::createSwapchain(const SwapchainCreateInfo& info)
{
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
    auto optimalImageExtent = findOptimalImageExtent(capabilities, window.get());
    auto optimalFormat      = findOptimalSurfaceFormat(formats);
    auto optimalPresentMode = findOptimalSurfacePresentMode(presentModes, info.presentMode);

    // Specify the number of images in the swapchain
    const uint32_t minImages = capabilities.minImageCount;
    const uint32_t maxImages = capabilities.maxImageCount == 0  // 0 means no limit on the image maximum.
                               ? std::numeric_limits<uint32_t>::max()
                               : capabilities.maxImageCount;
    numFrames = glm::clamp(info.imageCount, minImages, maxImages);

    if (info.throwWhenUnsupported)
    {
        if (optimalPresentMode != info.presentMode)
        {
            throw std::runtime_error(
                "[In Swapchain::createSwapchain]: Present mode \""
                + vk::to_string(info.presentMode) + " is not supported!"
            );
        }

        if (numFrames != info.imageCount)
        {
            throw std::runtime_error(
                "[In Swapchain::createSwapchain]: Image count of " + std::to_string(info.imageCount)
                + " is not supported!"
            );
        }
    }

    // Enable default image usage flags
    auto usageFlags = info.imageUsage;
    usageFlags |= vk::ImageUsageFlagBits::eColorAttachment;

    // Create the swapchain
    vk::SwapchainCreateInfoKHR createInfo(
        {},
        *surface,
        numFrames,
        optimalFormat.format,
        optimalFormat.colorSpace,
        optimalImageExtent,
        1u, // array layers
        usageFlags,
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
    currentFrame = 0;

    // Retrieve created images
    images = device->getSwapchainImagesKHR(swapchain.get());
    imageViews = createImageViews();

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
