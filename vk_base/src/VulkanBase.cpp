#include "VulkanBase.h"

#include <IL/il.h>



void vkb::vulkanInit(const VulkanInitInfo& initInfo)
{
    VulkanBase::init(initInfo);
}

void vkb::vulkanTerminate()
{
    VulkanBase::destroy();
}



void vkb::VulkanBase::onInit(std::function<void(void)> callback)
{
    if (!_isInitialized)
        onInitCallbacks.emplace_back(std::move(callback));
}

void vkb::VulkanBase::onDestroy(std::function<void(void)> callback)
{
    onDestroyCallbacks.emplace_back(std::move(callback));
}

void vkb::VulkanBase::init(const VulkanInitInfo& initInfo)
{
    if (isInitialized())
    {
        std::cout << "Warning: Tried to initialize VulkanBase more than once\n";
        return;
    }

    // Init GLFW first
    if (glfwInit() == GLFW_FALSE) {
        throw std::runtime_error("Initialization of GLFW failed!\n");
    }
    if constexpr (enableVerboseLogging) {
        std::cout << "GLFW initialized successfully\n";
    }

    // Initi DevIL
    ilInit();

    try {
        instance = std::make_unique<VulkanInstance>();
        Surface surface = createSurface(initInfo.windowSize);

        physicalDevice = std::make_unique<PhysicalDevice>(
            device_helpers::getOptimalPhysicalDevice(**instance, *surface.surface)
        );
        device = std::make_unique<Device>(*physicalDevice);
        swapchain = std::make_unique<Swapchain>(*device, std::move(surface));
        queueProvider = std::make_unique<QueueProvider>(*device);
    }
    catch (const std::exception& err) {
        std::cout << "Exception during vulkan_base initialization: " << err.what() << "\n";
        throw err;
    }

    _isInitialized = true;

    for (auto& func : onInitCallbacks) {
        std::invoke(func);
    }
    onInitCallbacks = {};
}

void vkb::VulkanBase::destroy()
{
    getDevice()->waitIdle();

    for (auto& func : onDestroyCallbacks) {
        std::invoke(func);
    }
    onDestroyCallbacks = {};

    try {
        queueProvider.reset();
        swapchain.reset();
        device.reset();
        instance.reset();
    }
    catch (const std::exception& err) {
        std::cout << "Exception in VulkanBase::destroy(): " << err.what() << "\n";
    }
}

bool vkb::VulkanBase::isInitialized() noexcept
{
    return _isInitialized;
}

auto vkb::VulkanBase::createSurface(vk::Extent2D size) -> Surface
{
    Surface result;

    // Create GLFW window
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    result.window = std::unique_ptr<GLFWwindow, Surface::windowDeleter>(
        glfwCreateWindow(size.width, size.height, "Hello :)", nullptr, nullptr),
        [](GLFWwindow* windowPtr) {
            glfwDestroyWindow(windowPtr);
            glfwTerminate();
        }
    );

    // Create Vulkan surface
    GLFWwindow* _window = result.window.get();
    VkSurfaceKHR _surface;
    if (glfwCreateWindowSurface(**instance, _window, nullptr, &_surface) != VK_SUCCESS) {
        throw std::runtime_error("Unable to create window surface!");
    }
    result.surface = std::unique_ptr<vk::SurfaceKHR, Surface::surfaceDeleter> {
        new vk::SurfaceKHR(_surface),
        [&](vk::SurfaceKHR* oldSurface) {
            (*instance)->destroySurfaceKHR(*oldSurface);
            delete oldSurface;
        }
    };

    return result;
}

auto vkb::VulkanBase::getInstance() noexcept -> VulkanInstance&
{
    return *instance;
}

auto vkb::VulkanBase::getPhysicalDevice() noexcept -> PhysicalDevice&
{
    return *physicalDevice;
}

auto vkb::VulkanBase::getDevice() noexcept -> Device&
{
    return *device;
}

auto vkb::VulkanBase::getSwapchain() noexcept -> Swapchain&
{
    return *swapchain;
}

auto vkb::VulkanBase::getQueueProvider() noexcept -> QueueProvider&
{
    return *queueProvider;
}
