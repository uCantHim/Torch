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
    const auto& graphicsFamilies = physicalDevice.queueCapabilities.graphicsCapable;
    const auto& presentationFamilies = physicalDevice.queueCapabilities.presentationCapable;
    if (graphicsFamilies.empty() || presentationFamilies.empty()) {
        throw std::runtime_error("Unable to create swapchain; no graphics or presentation queues available."
            "This should have been checked during device selection.");
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

        if constexpr (vkb::enableVerboseLogging) {
            std::cout << "Exclusive image sharing mode enabled\n";
        }
    }
    else
    {
        // The graphics and the presentation queues are of different families.
        result.imageSharingMode = vk::SharingMode::eConcurrent;
        result.imageSharingQueueFamilies = { graphics.index, present.index };

        if constexpr (vkb::enableVerboseLogging) {
            std::cout << "Concurrent image sharing mode enabled\n";
        }
    }

    return result;
}

auto findOptimalImageExtent(const vk::SurfaceCapabilitiesKHR& capabilities, GLFWwindow* window) -> vk::Extent2D
{
    auto& extent = capabilities.currentExtent;
    if (extent.width != UINT32_MAX && extent.height != UINT32_MAX) {
        // The extent has already been determined by the implementation
        if constexpr (vkb::enableVerboseLogging)
        {
            std::cout << "Using implementation defined image extent: ("
                << extent.width << ", " << extent.height << ")\n";
        }
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

    if constexpr (vkb::enableVerboseLogging) {
        std::cout << "Using swapchain image extent ("
            << extent.width << ", " << extent.height << ")\n";
    }

    return actualExtent;
}

auto findOptimalSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& formats)
    -> vk::SurfaceFormatKHR
{
    for (const auto& format : formats) {
        if (format.format == vk::Format::eR8G8B8A8Unorm
            && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
        {
            if constexpr (vkb::enableVerboseLogging) {
                std::cout << "Found optimal surface format \"" << vk::to_string(format.format)
                    << "\" in the color space \"" << vk::to_string(format.colorSpace) << "\"\n";
            }
            return format;
        }
    }

    if constexpr (vkb::enableVerboseLogging) {
        std::cout << "Picked suboptimal surface format.\n";
    }
    return formats[0];
}

auto findOptimalSurfacePresentMode(const std::vector<vk::PresentModeKHR>& presentModes)
    -> vk::PresentModeKHR
{
    for (const auto& mode : presentModes)
    {
        if (mode == vk::PresentModeKHR::eMailbox)
        {
            // Triple buffered. Not guaranteed to be supported.
            if constexpr (vkb::enableVerboseLogging) {
                std::cout << "Found optimal present mode: " << vk::to_string(mode) << "\n";
            }
            return mode;
        }
    }

    if constexpr (vkb::enableVerboseLogging) {
        std::cout << "Using suboptimal present mode: "
            << vk::to_string(vk::PresentModeKHR::eImmediate) << "\n";
    }
    return vk::PresentModeKHR::eImmediate;
}



// ---------------------------- //
//        Swapchain class       //
// ---------------------------- //

vkb::Swapchain::Swapchain(const Device& device, Surface s)
    :
    device(device),
    window(std::move(s.window)),
    surface(std::move(s.surface))
{
    initGlfwCallbacks(window.get());
    createSwapchain();
}

auto vkb::Swapchain::isOpen() const noexcept -> bool
{
    return isWindowOpen;
}

auto vkb::Swapchain::getGlfwWindow() const noexcept -> GLFWwindow*
{
    return window.get();
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
    vk::PresentInfoKHR presentInfo(
        static_cast<uint32_t>(waitSemaphores.size()), waitSemaphores.data(),
        1u, &swapchain.get(),
        &image,
        nullptr
    );

    try {
        auto result = queue.presentKHR(presentInfo);
        if (result == vk::Result::eSuboptimalKHR) {
            std::cout << "--- Swapchain has become suboptimal\n";
        }
    }
    catch (const vk::OutOfDateKHRError&) {
        if constexpr (enableVerboseLogging) {
            std::cout << "\n--- Swapchain has become invalid, create a new one.\n";
        }
        createSwapchain();
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

    double yInv = static_cast<double>(swapchain->getImageExtent().height) - ypos;
    vkb::EventHandler<vkb::MouseMoveEvent>::notify(
        { swapchain, static_cast<float>(xpos), static_cast<float>(yInv) }
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

void onWindowClose(GLFWwindow* window)
{
    auto swapchain = static_cast<vkb::Swapchain*>(glfwGetWindowUserPointer(window));
    swapchain->isWindowOpen = false;
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

    glfwSetWindowCloseCallback(window, onWindowClose);
}

void vkb::Swapchain::createSwapchain()
{
    // Signal start of recreation
    // This allows objects depending on the swapchain to prepare the
    // recreate, like locking resources.
    EventHandler<PreSwapchainRecreateEvent>::notifySync({ {this} });

    const auto timerStart = system_clock::now();
    const auto& physDevice = device.getPhysicalDevice();

    const auto [capabilities, formats, presentModes] = physDevice.getSwapchainSupport(*surface);
    auto optimalImageExtent = findOptimalImageExtent(capabilities, window.get());
    auto optimalFormat      = findOptimalSurfaceFormat(formats);
    auto optimalPresentMode = findOptimalSurfacePresentMode(presentModes);

    // Specify the number of images in the swapchain
    numFrames = capabilities.minImageCount + 1; // min + 1 is a good heuristic.
    if (capabilities.maxImageCount > 0) {
        // MaxImageCount == 0 would mean that there is no limit on the image maximum.
        numFrames = std::min(numFrames, capabilities.maxImageCount);
    }

    const auto [imageSharingMode, imageSharingQueueFamilies] = findOptimalImageSharingMode(physDevice);

    // Create the swapchain
    vk::SwapchainCreateInfoKHR createInfo(
        {},
        *surface,
        numFrames,
        optimalFormat.format,
        optimalFormat.colorSpace,
        optimalImageExtent,
        1u, // array layers
        vk::ImageUsageFlagBits::eColorAttachment,
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
    currentFrame = 0;

    // Retrieve created images
    images = device->getSwapchainImagesKHR(swapchain.get());
    imageViews = createImageViews();

    if constexpr (enableVerboseLogging) {
        std::cout << "New swapchain created, recreating swapchain-dependent resources...\n";
    }
    // Signal that recreation is finished.
    // Objects depending on the swapchain should now recreate their
    // resources.
    EventHandler<SwapchainRecreateEvent>::notifySync({ {this} });

    if constexpr (enableVerboseLogging)
    {
        auto duration = duration_cast<milliseconds>(system_clock::now() - timerStart);
        std::cout << "Swapchain has been created successfully with " << numFrames
           << " images (" << duration.count() << " ms)\n";
    }
}
