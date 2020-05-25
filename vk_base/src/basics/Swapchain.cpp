#include "Swapchain.h"

#include <chrono>
using namespace std::chrono;

#include "PhysicalDevice.h"
#include "Device.h"
#include "VulkanBase.h"



struct ImageSharingDetails {
    vk::SharingMode imageSharingMode{};
    std::vector<uint32_t> imageSharingQueueFamilies;
};

auto findOptimalImageSharingMode(const vkb::PhysicalDevice& physicalDevice) -> ImageSharingDetails
{
    ImageSharingDetails result;

    std::vector<uint32_t> imageSharingQueueFamilies;
    auto graphicsFamilies = physicalDevice.queueFamilies.graphicsFamilies;
    auto presentationFamilies = physicalDevice.queueFamilies.presentationFamilies;
    if (graphicsFamilies.empty() || presentationFamilies.empty()) {
        throw std::runtime_error("Unable to create swapchain; no graphics or presentation queues available."
            "This should have been checked during device selection.");
    }

    auto& graphics = graphicsFamilies[0];
    auto& present = presentationFamilies[0];
    if (graphics.index == present.index) {
        // The graphics and the presentation queues are of the same family.
        // This is preferred because exclusive access for one queue family
        // will likely be faster than concurrent access.
        result.imageSharingMode = vk::SharingMode::eExclusive;
        result.imageSharingQueueFamilies = {}; // Not important for exclusive sharing

        if constexpr (vkb::enableVerboseLogging) {
            std::cout << "Exclusive image sharing mode enabled\n";
        }
    }
    else {
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
    createSwapchain(false);
}


auto vkb::Swapchain::getImageExtent() const noexcept -> const vk::Extent2D&
{
    return swapchainExtent;
}


auto vkb::Swapchain::getImageFormat() const noexcept -> const vk::Format&
{
    return swapchainFormat;
}


auto vkb::Swapchain::getFrameCount() const noexcept -> uint32_t
{
    return static_cast<uint32_t>(numFrames);
}


auto vkb::Swapchain::getCurrentFrame() const noexcept -> uint32_t
{
    return FrameCounter::currentFrame;
}


auto vkb::Swapchain::getImage(uint32_t index) const noexcept -> const vk::Image&
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
    const vk::Queue& queue,
    const std::vector<vk::Semaphore>& waitSemaphores)
{
    vk::PresentInfoKHR presentInfo(
        static_cast<uint32_t>(waitSemaphores.size()), waitSemaphores.data(),
        1u, &swapchain.get(),
        &image,
        nullptr
    );
    try {
        queue.presentKHR(presentInfo);
    }
    catch (const vk::OutOfDateKHRError&) {
        if constexpr (enableVerboseLogging) {
            std::cout << "Swapchain has become invalid, create a new one.\n";
        }
        createSwapchain(true);
        return;
    }

    FrameCounter::currentFrame = (FrameCounter::currentFrame + 1) % numFrames;
}


void vkb::Swapchain::createSwapchain(bool recreate)
{
    auto timerStart = system_clock::now();

    // Signal that the swapchain is baout to be recreated.
    // This should be used by the dependent resources to ensure that no
    // access to the swapchain is performed until the recreation is
    // finished.
    if (recreate) {
        SwapchainDependentResource::DependentResourceLock::startRecreate();
    }

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

    // Retrieve created images
    images = device->getSwapchainImagesKHR(swapchain.get());

    swapchainExtent = optimalImageExtent;
    swapchainFormat = optimalFormat.format;
    FrameCounter::currentFrame = 0;

    if constexpr (enableVerboseLogging) {
        std::cout << "Created new swapchain, recreating dependent resources...\n";
    }

    // Recreate all dependent resources
    // Signal to all dependent resources that recreation is done
    if (recreate)
    {
        SwapchainDependentResource::DependentResourceLock::recreateAll(*this);
        SwapchainDependentResource::DependentResourceLock::endRecreate();
    }

    if constexpr (enableVerboseLogging)
    {
        auto duration = duration_cast<milliseconds>(system_clock::now() - timerStart);
        std::cout << "Swapchain has been created successfully (" << duration.count() << " ms)\n";
    }
}



void vkb::SwapchainDependentResource::DependentResourceLock::startRecreate()
{
    SwapchainDependentResource::isRecreating = true;

    for (auto res : SwapchainDependentResource::dependentResources)
    {
        if (res != nullptr) {
            res->signalRecreateRequired();
        }
    }
}

void vkb::SwapchainDependentResource::DependentResourceLock::recreateAll(Swapchain& swapchain)
{
    for (auto res : SwapchainDependentResource::dependentResources)
    {
        if (res != nullptr) {
            res->recreate(swapchain);
        }
    }
}

void vkb::SwapchainDependentResource::DependentResourceLock::endRecreate()
{
    for (auto res : SwapchainDependentResource::dependentResources)
    {
        if (res != nullptr) {
            res->signalRecreateFinished();
        }
    }

    for (auto newRes : newResources) {
        dependentResources.push_back(newRes);
    }
    newResources.clear();

    SwapchainDependentResource::isRecreating = false;
}



vkb::SwapchainDependentResource::SwapchainDependentResource()
    :
    index(registerSwapchainDependentResource(*this))
{
}

vkb::SwapchainDependentResource::~SwapchainDependentResource()
{
    removeSwapchainDependentResource(index);
}

uint32_t vkb::SwapchainDependentResource::registerSwapchainDependentResource(SwapchainDependentResource& res)
{
    if (isRecreating)
    {
        // The swapchain is being recreated, add the new object
        // after the recreation has finished
        newResources.push_back(&res);
        return dependentResources.size() + newResources.size() - 1;
    }
    else
    {
        dependentResources.push_back(&res);
        return dependentResources.size() - 1;
    }
}

void vkb::SwapchainDependentResource::removeSwapchainDependentResource(uint32_t index)
{
    assert(index < dependentResources.size());
    dependentResources[index] = nullptr;
}

