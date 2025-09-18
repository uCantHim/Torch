#include "trc/base/VulkanInstance.h"

#include <cassert>

#include <GLFW/glfw3.h>
#include <trc_util/algorithm/VectorTransform.h>

#include "trc/base/Logging.h"
#include "trc/base/PhysicalDevice.h"
#include "trc/base/Swapchain.h"
#include "trc/base/VulkanDebug.h"



auto getRequiredInstanceExtensions(bool useGlfw) -> std::vector<const char*>
{
    std::vector<const char*> extensions;

    if (useGlfw)
    {
        uint32_t requiredExtensionCount = 0;
        auto requiredExtensions = glfwGetRequiredInstanceExtensions(&requiredExtensionCount);
        if (requiredExtensions == nullptr)
        {
            trc::log::warn << trc::log::here()
                << ": GLFW is enabled but no valid set of instance extensions was found that"
                << " would allow window surface creation."
                << " Vulkan can still be used for compute/off-screen tasks.";
        }
        else {
            assert(requiredExtensionCount > 0);
            extensions = { requiredExtensions, requiredExtensions + requiredExtensionCount };
        }
    }

#ifdef TRC_DEBUG
    extensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

    return extensions;
}

trc::VulkanInstance::VulkanInstance(const VulkanInstanceCreateInfo& createInfo)
{
    // Init GLFW first
    const int glfwStatus = glfwInit();
    if (glfwStatus == GLFW_FALSE)
    {
        const char* errorMsg{ nullptr };
        glfwGetError(&errorMsg);
        log::info << "GLFW initialization failed: " << errorMsg;
    }
    else {
        log::info << "GLFW initialized successfully";
    }

    // Count the number of created instances so that we know when to terminate GLFW.
    // This counter is decremented in a unique_ptr deleter because that makes it
    // safe (and easier) to move VulkanInstance.
    ++numExistingInstances;
    glfwAlivenessChecker = {
        new std::byte{42},
        [](std::byte* b) {
            delete b;

            // Check whether we can terminate GLFW.
            --numExistingInstances;
            if (numExistingInstances == 0) {
                glfwTerminate();
            }
        }
    };

    const auto layers = getRequiredValidationLayers();
    const auto extensions = trc::util::merged(
        getRequiredInstanceExtensions(glfwStatus == GLFW_TRUE),
        createInfo.instanceExtensions
    );

    const vk::ApplicationInfo appInfo(
        createInfo.appName.c_str(), createInfo.appVersion,
        createInfo.engineName.c_str(), createInfo.engineVersion,
        createInfo.vulkanApiVersion
    );

    vk::StructureChain chain{
        vk::InstanceCreateInfo({}, &appInfo, layers, extensions),
#ifdef TRC_DEBUG
        vk::ValidationFeaturesEXT(
            createInfo.enabledValidationFeatures,
            createInfo.disabledValidationFeatures
        ),
#endif
    };

    instance = vk::createInstanceUnique(chain.get());

    log::info << "Vulkan instance created successfully.";
    log::info << "   Enabled validation layers:";
    for (const auto& name : layers) {
        log::info << "    - " << name;
    }
    log::info << "   Enabled instance extensions:";
    for (const auto& name : extensions) {
        log::info << "    - " << name;
    }

    debug = std::make_unique<VulkanDebug>(*instance);
}

auto trc::VulkanInstance::makeSurface(const SurfaceCreateInfo& createInfo)
    -> std::expected<Surface, std::string>
{
    try {
        return Surface{ *instance, createInfo };
    }
    catch (const std::runtime_error& err) {
        return std::unexpected(err.what());
    }
}

auto trc::VulkanInstance::queryPhysicalDevices(std::optional<vk::SurfaceKHR> surface)
    -> std::vector<PhysicalDevice>
{
    return findAllPhysicalDevices(*instance, surface);
}
