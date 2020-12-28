#include "VulkanBase.h"

#include <IL/il.h>

#include "event/EventHandler.h"
#include "StaticInit.h"



void vkb::vulkanInit(const VulkanInitInfo& initInfo)
{
    VulkanBase::init(initInfo);
}

void vkb::vulkanTerminate()
{
    VulkanBase::destroy();
}

auto vkb::createSurface(VulkanInstance& instance, SurfaceCreateInfo createInfo) -> Surface
{
    Surface result;

    // Create GLFW window
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    result.window = std::unique_ptr<GLFWwindow, Surface::windowDeleter>(
        glfwCreateWindow(
            createInfo.windowSize.width, createInfo.windowSize.height,
            createInfo.windowTitle.c_str(),
            nullptr, nullptr
        ),
        [](GLFWwindow* windowPtr) {
            glfwDestroyWindow(windowPtr);
            glfwTerminate();
        }
    );

    // Create Vulkan surface
    GLFWwindow* _window = result.window.get();
    VkSurfaceKHR _surface;
    if (glfwCreateWindowSurface(*instance, _window, nullptr, &_surface) != VK_SUCCESS) {
        throw std::runtime_error("Unable to create window surface!");
    }
    result.surface = std::unique_ptr<vk::SurfaceKHR, Surface::surfaceDeleter> {
        new vk::SurfaceKHR(_surface),
        [&](vk::SurfaceKHR* oldSurface) {
            instance->destroySurfaceKHR(*oldSurface, {});
            delete oldSurface;
        }
    };

    return result;
}



void vkb::VulkanBase::init(const VulkanInitInfo& initInfo)
{
    if (isInitialized)
    {
        std::cout << "Warning: Tried to initialize VulkanBase more than once\n";
        return;
    }
    isInitialized = true;

    EventThread::start();

    // Init GLFW first
    if (glfwInit() == GLFW_FALSE) {
        throw std::runtime_error("Initialization of GLFW failed!\n");
    }
    if constexpr (enableVerboseLogging) {
        std::cout << "GLFW initialized successfully\n";
    }

    // Initi DevIL
    ilInit();

    if (initInfo.createResources)
    {
        try {
            instance = std::make_unique<VulkanInstance>();
            Surface surface = createSurface(*instance, initInfo.surfaceCreateInfo);

            physicalDevice = std::make_unique<PhysicalDevice>(
                device_helpers::getOptimalPhysicalDevice(**instance, *surface.surface)
            );
            device = std::make_unique<Device>(*physicalDevice, initInfo.deviceExtensions);
            swapchain = std::make_unique<Swapchain>(*device, std::move(surface));
        }
        catch (const std::exception& err) {
            std::cout << "Exception during vulkan_base initialization: " << err.what() << "\n";
            throw err;
        }
    }

    StaticInit::executeStaticInitializers();
}

void vkb::VulkanBase::destroy()
{
    EventThread::terminate();
    getDevice()->waitIdle();

    StaticInit::executeStaticDestructors();

    try {
        swapchain.reset();
        device.reset();
        instance.reset();
    }
    catch (const std::exception& err) {
        std::cout << "Exception in VulkanBase::destroy(): " << err.what() << "\n";
    }
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
