#include "Torch.h"

#include "TorchResources.h"



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
    if (torchGlobalVulkanInstance == nullptr)
    {
        throw std::runtime_error(
            "[In trc::getVulkanInstance]: Instance not initialized. You must call init() first!"
        );
    }

    return *torchGlobalVulkanInstance;
}

auto trc::initFull() -> TorchStack
{
    init();

    auto instance = std::make_unique<trc::Instance>();
    auto window = instance->makeWindow({});
    auto ar = std::make_unique<AssetRegistry>(*instance);
    auto sp = std::make_unique<ShadowPool>(*window, ShadowPoolCreateInfo{ .maxShadowMaps=200 });
    auto config{
        std::make_unique<DeferredRenderConfig>(
            DeferredRenderCreateInfo{
                *instance,
                *window,
                ar.get(),
                sp.get(),
                3  // max transparent frags
            }
        )
    };

    return {
        std::move(instance),
        std::move(window),
        std::move(ar),
        std::move(sp),
        std::move(config)
    };
}

void trc::terminate()
{
    torchGlobalVulkanInstance.reset();

    RenderStageType::destroyAll();
    vkb::vulkanTerminate();
}



auto trc::TorchStack::makeDrawConfig(Scene& scene, Camera& camera) const -> DrawConfig
{
    return {
        .scene        = &scene,
        .camera       = &camera,
        .renderConfig = renderConfig.get(),
        .renderAreas  = { window->makeFullscreenRenderArea() }
    };
}

void trc::TorchStack::drawFrame(const DrawConfig& draw)
{
    window->drawFrame(draw);
}
