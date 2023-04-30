// If I enable vk::ValidationFeatureEnableEXT::eSynchronizationValidation,
// I get very weird WRITE-AFTER-WRITE hazard errors that are, how I see it,
// not sensible.
//
// The following code reproduces the error for an image layout change before a
// clear command:
//
// ```c++
//    trc::Image image(device, 20, 20);

//    device.executeCommands(trc::QueueType::graphics, [&](vk::CommandBuffer cmdBuf)
//    {
//        const vk::ImageMemoryBarrier2 imageBarrier(
//            vk::PipelineStageFlagBits2::eAllCommands, vk::AccessFlagBits2::eMemoryWrite | vk::AccessFlagBits2::eTransferWrite,
//            vk::PipelineStageFlagBits2::eClear, vk::AccessFlagBits2::eTransferWrite | vk::AccessFlagBits2::eMemoryWrite,
//            vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal,
//            VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
//            *image,
//            vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)
//        );
//        cmdBuf.pipelineBarrier2(
//            vk::DependencyInfo{
//                vk::DependencyFlagBits::eByRegion,
//                {}, {}, imageBarrier
//            }
//        );

//        cmdBuf.clearColorImage(
//            *image,
//            vk::ImageLayout::eTransferDstOptimal,
//            vk::ClearColorValue(0, 0, 0, 0),
//            vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)
//        );
//    });
// ```

#include "trc/base/VulkanInstance.h"

#include <cassert>

#include <GLFW/glfw3.h>
#include <trc_util/algorithm/VectorTransform.h>

#include "trc/base/Logging.h"
#include "trc/base/VulkanDebug.h"



std::vector<const char*> getRequiredInstanceExtensions()
{
    std::vector<const char*> extensions;

    uint32_t requiredExtensionCount = 0;
    auto requiredExtensions = glfwGetRequiredInstanceExtensions(&requiredExtensionCount);
    if (requiredExtensions == nullptr)
    {
        trc::log::error << trc::log::here() << ": The current machine does not support the minimal"
            " required set of Vulkan extensions. Surface creation will not be possible.";
    }
    else {
        assert(requiredExtensionCount > 0);
        extensions = { requiredExtensions, requiredExtensions + requiredExtensionCount };
    }

#ifdef TRC_DEBUG
    extensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

    return extensions;
}

trc::VulkanInstance::VulkanInstance(const VulkanInstanceCreateInfo& createInfo)
{
    const auto layers = getRequiredValidationLayers();
    const auto extensions = trc::util::merged(
        getRequiredInstanceExtensions(),
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
