#include "trc/base/Swapchain.h"

#include <cassert>
#include <stdexcept>
using namespace std::chrono;

#include <trc_util/Assert.h>
#include <trc_util/Timer.h>

#include "trc/base/Device.h"
#include "trc/base/Logging.h"
#include "trc/base/PhysicalDevice.h"



auto toGlfwEnum(trc::CursorShape shape) -> int
{
    switch (shape)
    {
    case trc::CursorShape::eArrow: return GLFW_ARROW_CURSOR;
    case trc::CursorShape::eBeam: return GLFW_IBEAM_CURSOR;
    case trc::CursorShape::eCrosshair: return GLFW_CROSSHAIR_CURSOR;
    case trc::CursorShape::ePointingHand: return GLFW_POINTING_HAND_CURSOR;
    case trc::CursorShape::eResize: return GLFW_RESIZE_ALL_CURSOR;
    case trc::CursorShape::eResizeHorizontal: return GLFW_RESIZE_EW_CURSOR;
    case trc::CursorShape::eResizeVertical: return GLFW_RESIZE_NS_CURSOR;
    case trc::CursorShape::eResizeDiagonalULtoLR: return GLFW_RESIZE_NWSE_CURSOR;
    case trc::CursorShape::eResizeDiagonalURtoLL: return GLFW_RESIZE_NESW_CURSOR;
    case trc::CursorShape::eNotAllowed: return GLFW_NOT_ALLOWED_CURSOR;
    }
}



trc::Surface::Surface(vk::Instance instance, const SurfaceCreateInfo& info)
{
    // Create GLFW window
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_VISIBLE,   !info.hidden);
    glfwWindowHint(GLFW_MAXIMIZED, info.maximized);
    glfwWindowHint(GLFW_RESIZABLE, info.resizeable);
    glfwWindowHint(GLFW_FLOATING,  info.floating);
    glfwWindowHint(GLFW_DECORATED, info.decorated);
    glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, info.transparentFramebuffer);
    window = std::unique_ptr<GLFWwindow, Surface::WindowDeleter>(
        glfwCreateWindow(
            info.windowSize.x, info.windowSize.y,
            info.windowTitle.c_str(),
            nullptr, nullptr
        ),
        [](GLFWwindow* windowPtr) {
            glfwDestroyWindow(windowPtr);
        }
    );

    if (window == nullptr)
    {
        const char* errorMsg{ nullptr };
        glfwGetError(&errorMsg);
        throw std::runtime_error("[In Surface::Surface]: Unable to create a window: "
                                 + std::string(errorMsg));
    }
    GLFWwindow* _window = window.get();

    // Create Vulkan surface
    VkSurfaceKHR _surface{ VK_NULL_HANDLE };
    if (glfwCreateWindowSurface(instance, _window, nullptr, &_surface) != VK_SUCCESS)
    {
        const char* errorMsg{ nullptr };
        glfwGetError(&errorMsg);
        throw std::runtime_error("[In Surface::Surface]: Unable to create window surface: "
                                 + std::string(errorMsg));
    }
    surface = std::unique_ptr<vk::SurfaceKHR, Surface::SurfaceDeleter> {
        new vk::SurfaceKHR(_surface),
        [instance](vk::SurfaceKHR* oldSurface) {
            instance.destroySurfaceKHR(*oldSurface);
            delete oldSurface;
        }
    };
}

auto trc::Surface::getGlfwWindow() -> GLFWwindow*
{
    return window.get();
}

auto trc::Surface::getVulkanSurface() const -> vk::SurfaceKHR
{
    return *surface;
}



struct ImageSharingDetails {
    vk::SharingMode imageSharingMode{};
    std::vector<uint32_t> imageSharingQueueFamilies;
};

auto findOptimalImageSharingMode(const trc::PhysicalDevice& physicalDevice) -> ImageSharingDetails
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

    int width{ 0 };
    int height{ 0 };
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
    { // logging
        auto line = trc::log::info << "   Possible surface formats: ";
        for (const auto& format : formats)
        {
            line << "(" << vk::to_string(format.format) << " - "
                 << vk::to_string(format.colorSpace) << "), ";
        }
        line << "\b\b ";
    } // logging

    for (const auto& format : formats)
    {
        if (format.format == vk::Format::eR8G8B8A8Unorm
            && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
        {
            trc::log::info << "   Found optimal surface format \"" << vk::to_string(format.format)
                << "\" in the color space \"" << vk::to_string(format.colorSpace) << "\"";
            return format;
        }
    }

    trc::log::info << "   Picked suboptimal surface format.";
    return formats[0];
}

auto findOptimalSurfacePresentMode(const std::vector<vk::PresentModeKHR>& presentModes,
                                   vk::PresentModeKHR preferredMode = vk::PresentModeKHR::eMailbox)
    -> vk::PresentModeKHR
{
    { // logging
        auto line = trc::log::info << "   Possible present modes: ";
        for (const auto& mode : presentModes) {
            line << vk::to_string(mode) << ", ";
        }
        line << "\b\b ";
    } // logging

    bool immediateSupported{ false };
    for (const auto& mode : presentModes)
    {
        if (mode == preferredMode)
        {
            trc::log::info << "   Using preferred present mode: " << vk::to_string(mode);
            return mode;
        }

        if (mode == vk::PresentModeKHR::eImmediate) {
            immediateSupported = true;
        }
    }

    trc::log::info << "   Preferred present mode is not supported!";
    if (immediateSupported) {
        return vk::PresentModeKHR::eImmediate;
    }
    return vk::PresentModeKHR::eFifo;  // The only mode that is required to be supported
}



// ---------------------------- //
//        Swapchain class       //
// ---------------------------- //

trc::Swapchain::Swapchain(
    const Device& device,
    Surface s,
    s_ptr<InputProcessor> _inputProcessor,
    const SwapchainCreateInfo& info)
    :
    FrameClock([&device, &s, &info] {
        const auto& phys = device.getPhysicalDevice();
        const auto capabilities = phys.getSwapchainSupport(s.getVulkanSurface()).surfaceCapabilities;
        const uint32_t minImages = capabilities.minImageCount;
        const uint32_t maxImages = capabilities.maxImageCount == 0  // 0 means no limit
                                   ? std::numeric_limits<uint32_t>::max()
                                   : capabilities.maxImageCount;

        const uint32_t numFrames = glm::clamp(info.imageCount, minImages, maxImages);
        if (numFrames != info.imageCount)
        {
            if (info.throwWhenUnsupported)
            {
                const auto msg = "Image count of " + std::to_string(info.imageCount) + " is not"
                                 " supported! The device supports a minimum of "
                                 + std::to_string(minImages) + " images and a maximum of "
                                 + std::to_string(maxImages) + " images.";

                log::error << log::here() << msg;
                throw std::runtime_error("[In Swapchain::Swapchain]: " + msg);
            }
            else {
                log::warn << log::here() << ": The preferred image count of " << info.imageCount
                          << " is not supported by the device. Creating swapchain with "
                          << numFrames << " images instead.";
            }
        }

        return numFrames;
    }()),
    device(device),
    createInfo(info),
    surface(std::move(s)),
    window(surface.getGlfwWindow()),
    inputProcessor(_inputProcessor),
    swapchainImageUsage([&info] {
        // Add necessary default usage flags
        return info.imageUsage | vk::ImageUsageFlagBits::eColorAttachment;
    }())
{
    assert_arg(_inputProcessor != nullptr);

    /**
     * This call also has practical significance: Vulkan requires that the
     * getSurfaceSupportKHR function be called on every surface before a
     * swapchain is created on it.
     */
    if (!device.getPhysicalDevice().hasSurfaceSupport(surface.getVulkanSurface()))
    {
        throw std::runtime_error(
            "In Swapchain::Swapchain(): Physical device does not have surface support for the"
            " specified surface! This means that none of the device's queue families returned"
            " true from a call to vkGetPhysicalDeviceSurfaceSupportKHR."
        );
    }

    glfwSetWindowUserPointer(window, this);

    // Register input event callbacks
    static auto getSwapchain = [](GLFWwindow* window) -> Swapchain& {
        assert(glfwGetWindowUserPointer(window) != nullptr);
        return *static_cast<Swapchain*>(glfwGetWindowUserPointer(window));
    };

    glfwSetCharCallback(window, [](auto win, uint32_t c){
        auto& sc = getSwapchain(win);
        sc.inputProcessor->onCharInput(sc, c);
    });
    glfwSetKeyCallback(window, [](auto win, int key, int, int action, int mods){
        auto& sc = getSwapchain(win);
        sc.inputProcessor->onKeyInput(sc,
                                      static_cast<Key>(key),
                                      static_cast<InputAction>(action),
                                      KeyModFlags(mods));
    });
    glfwSetCursorEnterCallback(window, [](auto win, int entered) {
        auto& sc = getSwapchain(win);
        sc.inputProcessor->onMouseEnter(sc, !!entered);
    });
    glfwSetCursorPosCallback(window, [](auto win, double xpos, double ypos){
        auto& sc = getSwapchain(win);
        sc.inputProcessor->onMouseMove(sc, xpos, ypos);
    });
    glfwSetMouseButtonCallback(window, [](auto win, int button, int action, int mods){
        auto& sc = getSwapchain(win);
        sc.inputProcessor->onMouseInput(sc,
                                        static_cast<MouseButton>(button),
                                        static_cast<InputAction>(action),
                                        KeyModFlags(mods));
    });
    glfwSetScrollCallback(window, [](auto win, double xoff, double yoff){
        auto& sc = getSwapchain(win);
        sc.inputProcessor->onMouseScroll(sc, xoff, yoff);
    });
    glfwSetWindowFocusCallback(window, [](auto win, int focused) {
        auto& sc = getSwapchain(win);
        sc.inputProcessor->onWindowFocus(sc, !!focused);
    });
    glfwSetWindowSizeCallback(window, [](auto win, int x, int y) {
        assert(x > 0 && y > 0);
        auto& sc = getSwapchain(win);
        sc.inputProcessor->onWindowResize(sc, x, y);
    });
    glfwSetWindowPosCallback(window, [](auto win, int x, int y) {
        auto& sc = getSwapchain(win);
        sc.inputProcessor->onWindowMove(sc, x, y);
    });
    glfwSetWindowCloseCallback(window, [](auto win){
        auto& sc = getSwapchain(win);
        sc.inputProcessor->onWindowClose(sc);
    });
    glfwSetWindowRefreshCallback(window, [](auto win) {
        auto& sc = getSwapchain(win);
        sc.inputProcessor->onWindowRefresh(sc);
    });

    resizeCallbacks.add([](Swapchain& sc) {
        const auto size = sc.getWindowSize();
        sc.inputProcessor->onWindowResize(sc, size.x, size.y);
    });

    // Create the Vulkan swapchain object
    createSwapchain(info);
}

auto trc::Swapchain::getDevice() const -> const Device&
{
    return device;
}

auto trc::Swapchain::acquireImage(vk::Semaphore signalSemaphore) const -> uint32_t
{
    auto result = device->acquireNextImageKHR(*swapchain, UINT64_MAX, signalSemaphore, vk::Fence());
    if (result.result == vk::Result::eErrorOutOfDateKHR)
    {
        // Recreate swapchain?
        log::error << "--- Image acquisition threw error! Investigate this since I have not"
                   << " decided what to do here! (in Swapchain::acquireImage())";
    }
    return result.value;
}

bool trc::Swapchain::presentImage(
    uint32_t image,
    vk::Queue queue,
    const std::vector<vk::Semaphore>& waitSemaphores)
{
    vk::PresentInfoKHR presentInfo(waitSemaphores, *swapchain, image);

    try {
        auto result = queue.presentKHR(presentInfo);
        if (result == vk::Result::eSuboptimalKHR) {
            log::info << "--- Swapchain has become suboptimal. Do nothing.";
        }
    }
    catch (const vk::OutOfDateKHRError&)
    {
        log::info << "--- Swapchain has become invalid, create a new one.";
        createSwapchain(createInfo);
        FrameClock::resetCurrentFrame();
        return false;
    }

    FrameClock::endFrame();
    return true;
}

void trc::Swapchain::addCallbackBeforeSwapchainRecreate(
    std::function<void(Swapchain&)> recreateCallback)
{
    beforeRecreateCallbacks.add(std::move(recreateCallback));
}

void trc::Swapchain::addCallbackAfterSwapchainRecreate(
    std::function<void(Swapchain&)> recreateCallback)
{
    afterRecreateCallbacks.add(std::move(recreateCallback));
}

void trc::Swapchain::addCallbackOnResize(std::function<void(Swapchain&)> resizeCallback)
{
    resizeCallbacks.add(std::move(resizeCallback));
}

auto trc::Swapchain::getImageExtent() const noexcept -> vk::Extent2D
{
    return swapchainExtent;
}

auto trc::Swapchain::getImageFormat() const noexcept -> vk::Format
{
    return swapchainFormat;
}

auto trc::Swapchain::getImageUsage() const noexcept -> vk::ImageUsageFlags
{
    return swapchainImageUsage;
}

auto trc::Swapchain::getImage(uint32_t index) const noexcept -> vk::Image
{
    return images[index];
}

auto trc::Swapchain::getImageView(uint32_t imageIndex) const noexcept -> vk::ImageView
{
    assert(imageIndex < getFrameCount());

    return *imageViews[imageIndex];
}

void trc::Swapchain::setPresentMode(vk::PresentModeKHR newMode)
{
    createInfo.presentMode = newMode;
    createSwapchain(createInfo);
}



////////////////////
//  Window stuff  //
////////////////////

auto trc::Swapchain::getGlfwWindow() const noexcept -> GLFWwindow*
{
    return window;
}

void trc::Swapchain::setInputProcessor(s_ptr<InputProcessor> newProc)
{
    assert_arg(newProc != nullptr);
    inputProcessor = newProc;
}

auto trc::Swapchain::isOpen() const noexcept -> bool
{
    return !glfwWindowShouldClose(window);
}

auto trc::Swapchain::shouldClose() const noexcept -> bool
{
    return !isOpen();
}

auto trc::Swapchain::getSize() const noexcept -> glm::uvec2
{
    return { swapchainExtent.width, swapchainExtent.height };
}

auto trc::Swapchain::getWindowSize() const -> glm::uvec2
{
    glm::ivec2 winSize;
    glfwGetWindowSize(window, &winSize.x, &winSize.y);
    return winSize;
}

void trc::Swapchain::resize(uint32_t width, uint32_t height)
{
    glfwSetWindowSize(window, width, height);

    // I don't have to recreate here. The next present will return an
    // out-of-date swapchain.
}

void trc::Swapchain::maximize()
{
    glfwMaximizeWindow(window);
}

void trc::Swapchain::minimize()
{
    glfwIconifyWindow(window);
}

auto trc::Swapchain::isMaximized() const -> bool
{
    return glfwGetWindowAttrib(window, GLFW_MAXIMIZED);
}

auto trc::Swapchain::isMinimized() const -> bool
{
    return glfwGetWindowAttrib(window, GLFW_ICONIFIED);
}

void trc::Swapchain::restore()
{
    glfwRestoreWindow(window);
}

auto trc::Swapchain::getAspectRatio() const noexcept -> float
{
    return static_cast<float>(swapchainExtent.width) / static_cast<float>(swapchainExtent.height);
}

void trc::Swapchain::forceAspectRatio(int32_t width, int32_t height)
{
    glfwSetWindowAspectRatio(window, width, height);
}

void trc::Swapchain::forceAspectRatio(const bool forced)
{
    if (forced) {
        forceAspectRatio(getSize().x, getSize().y);
    }
    else {
        forceAspectRatio(GLFW_DONT_CARE, GLFW_DONT_CARE);
    }
}

void trc::Swapchain::hide()
{
    glfwHideWindow(window);
}

void trc::Swapchain::show()
{
    glfwShowWindow(window);
}

void trc::Swapchain::setUserResizeable(const bool resizeable)
{
    glfwSetWindowAttrib(window, GLFW_RESIZABLE, resizeable);
}

auto trc::Swapchain::isUserResizeable() const -> bool
{
    return glfwGetWindowAttrib(window, GLFW_RESIZABLE);
}

void trc::Swapchain::setDecorated(bool decorated)
{
    glfwSetWindowAttrib(window, GLFW_DECORATED, decorated);
}

auto trc::Swapchain::getDecorated() const -> bool
{
    return glfwGetWindowAttrib(window, GLFW_DECORATED);
}

void trc::Swapchain::setFloating(bool floating)
{
    glfwSetWindowAttrib(window, GLFW_FLOATING, floating);
}

auto trc::Swapchain::getFloating() const -> bool
{
    return glfwGetWindowAttrib(window, GLFW_FLOATING);
}

void trc::Swapchain::setOpacity(float opacity)
{
    glfwSetWindowOpacity(window, opacity);
}

auto trc::Swapchain::getOpacity() const -> float
{
    return glfwGetWindowOpacity(window);
}

auto trc::Swapchain::getPosition() const -> glm::ivec2
{
    glm::ivec2 pos;
    glfwGetWindowPos(window, &pos.x, &pos.y);
    return pos;
}

void trc::Swapchain::setPosition(int32_t x, int32_t y)
{
    glfwSetWindowPos(window, x, y);
}

void trc::Swapchain::setTitle(const char* title)
{
    glfwSetWindowTitle(window, title);
}

auto trc::Swapchain::getKeyState(Key key) const -> InputAction
{
    return static_cast<InputAction>(glfwGetKey(window, static_cast<int>(key)));
}

auto trc::Swapchain::getMouseButtonState(MouseButton button) const -> InputAction
{
    return static_cast<InputAction>(glfwGetMouseButton(window, static_cast<int>(button)));
}

auto trc::Swapchain::getKeyModifierState() const -> KeyModFlags
{
    constexpr auto none = KeyModFlagBits::none;

    const auto window = getGlfwWindow();
    return
        (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) ? KeyModFlagBits::shift : none)
        | (glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) ? KeyModFlagBits::shift : none)
        | (glfwGetKey(window, GLFW_KEY_LEFT_ALT) ? KeyModFlagBits::alt : none)
        | (glfwGetKey(window, GLFW_KEY_RIGHT_ALT) ? KeyModFlagBits::alt : none)
        | (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) ? KeyModFlagBits::control : none)
        | (glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) ? KeyModFlagBits::control : none)
        | (glfwGetKey(window, GLFW_KEY_LEFT_SUPER) ? KeyModFlagBits::super : none)
        | (glfwGetKey(window, GLFW_KEY_RIGHT_SUPER) ? KeyModFlagBits::super : none)
        | (glfwGetKey(window, GLFW_KEY_CAPS_LOCK) ? KeyModFlagBits::caps_lock : none)
        | (glfwGetKey(window, GLFW_KEY_NUM_LOCK) ? KeyModFlagBits::num_lock : none);
}

auto trc::Swapchain::getMousePosition() const -> glm::vec2
{
    glm::dvec2 pos;
    glfwGetCursorPos(window, &pos.x, &pos.y);

    return pos;
}

auto trc::Swapchain::getMousePositionLowerLeft() const -> glm::vec2
{
    glm::dvec2 pos;
    glfwGetCursorPos(window, &pos.x, &pos.y);

    return { pos.x, static_cast<double>(getSize().y) - pos.y };
}

auto trc::Swapchain::isPressed(Key key) const -> bool
{
    return getKeyState(key) == InputAction::press;
}

auto trc::Swapchain::isPressed(MouseButton button) const -> bool
{
    return getMouseButtonState(button) == InputAction::press;
}

bool trc::Swapchain::systemSupportsCursor(CursorShape shape)
{
    return !!cursors.tryGet(shape);
}

bool trc::Swapchain::setCursorShape(CursorShape shape)
{
    if (auto cursor = cursors.tryGet(shape))
    {
        glfwSetCursor(getGlfwWindow(), cursor);
        return true;
    }
    return false;
}

auto trc::Swapchain::CursorStorage::tryGet(CursorShape shape) -> GLFWcursor*
{
    auto& cursor = standardCursors[std::to_underlying(shape)];
    if (cursor == nullptr) {
        cursor = glfwCreateStandardCursor(toGlfwEnum(shape));
    }
    return cursor;
}



//////////////////////////
//  Swapchain creation  //
//////////////////////////

void trc::Swapchain::createSwapchain(const SwapchainCreateInfo& info)
{
    std::scoped_lock lock(swapchainRecreateLock);
    log::info << "Starting swapchain creation";

    // Signal start of recreation
    // This allows objects depending on the swapchain to prepare the
    // recreate, like locking resources.
    beforeRecreateCallbacks.call(*this);

    Timer timer;
    const auto& physDevice = device.getPhysicalDevice();

    const auto [imageSharingMode, imageSharingQueueFamilies] = findOptimalImageSharingMode(physDevice);
    const auto [capabilities, formats, presentModes] = physDevice.getSwapchainSupport(surface.getVulkanSurface());
    const auto optimalImageExtent = findOptimalImageExtent(capabilities, window);
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
        surface.getVulkanSurface(),
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

    // Retrieve created images and create image views
    images = device->getSwapchainImagesKHR(*swapchain);
    imageViews.clear();
    for (size_t i = 0; i < images.size(); i++)
    {
        imageViews.emplace_back(createImageView(i));
        device.setDebugName(images[i], "Swapchain image #" + std::to_string(i));
        device.setDebugName(*imageViews[i], "Swapchain image view #" + std::to_string(i));
    }

    // Logging
    {
        log::info << "Swapchain created (" << timer.duration() << " ms):";

        log::info << "   Size: (" << swapchainExtent.width << ", " << swapchainExtent.height << ")";
        log::info << "   Images: " << numFrames;
        log::info << "   Format: " << vk::to_string(optimalFormat.format)
                  << ", Color Space: " << vk::to_string(optimalFormat.colorSpace);
        log::info << "   Image usage: " << vk::to_string(createInfo.imageUsage);
        log::info << "   Image sharing mode: " << vk::to_string(imageSharingMode);
        log::info << "   Present mode: " << vk::to_string(optimalPresentMode);

        log::info << "Recreating swapchain-dependent resources...";
    }

    // Signal that recreation is finished.
    // Objects depending on the swapchain should now recreate their
    // resources.
    timer.reset();
    afterRecreateCallbacks.call(*this);
    resizeCallbacks.call(*this);

    log::info << "Swapchain-dependent resources created (" << timer.duration() << "ms)";
}

auto trc::Swapchain::createImageView(uint32_t imageIndex) const -> vk::UniqueImageView
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
