#include "Torch.h"



static inline trc::u_ptr<vkb::VulkanInstance> torchGlobalVulkanInstance{ nullptr };



void trc::init(const TorchInitInfo&)
{
    static bool init{ false };
    if (init) return;
    init = true;

    /**
     * Initialize:
     *   - Event thread
     *   - GLFW
     *   - DevIL
     */
    vkb::vulkanInit({
        .createResources=false,
        .delayStaticInitializerExecution=true,
    });

    torchGlobalVulkanInstance = std::make_unique<vkb::VulkanInstance>();
}

auto trc::getVulkanInstance() -> vkb::VulkanInstance&
{
    return *torchGlobalVulkanInstance;
}

auto trc::initDefault() -> DefaultTorchStack
{
    init();

    auto instance{ trc::createDefaultInstance() };
    auto window{ instance->makeWindow({}) };
    auto ar{ std::make_unique<AssetRegistry>(*instance) };
    auto config{
        std::make_unique<DeferredRenderConfig>(
            DeferredRenderCreateInfo{
                *instance,
                *window,
                ar.get(),
                3  // max transparent frags
            }
        )
    };

    Queues::init(instance->getQueueManager());

    return {
        std::move(instance),
        std::move(window),
        std::move(ar),
        std::move(config)
    };
}

void trc::terminate()
{
    torchGlobalVulkanInstance.reset();

    RenderStageType::destroyAll();
    vkb::vulkanTerminate();
}
