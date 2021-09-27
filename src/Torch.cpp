#include "Torch.h"

#include <IL/il.h>

#include "TorchResources.h"



static inline trc::u_ptr<vkb::VulkanInstance> torchGlobalVulkanInstance{ nullptr };



void trc::init(const TorchInitInfo&)
{
    static bool init{ false };
    if (init) return;
    init = true;

    vkb::init();

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

auto trc::initFull(
    const InstanceCreateInfo& instanceInfo,
    const WindowCreateInfo& windowInfo
    ) -> TorchStack
{
    init();

    auto instance = std::make_unique<trc::Instance>(instanceInfo);

    auto winInfo = windowInfo;
    if (instanceInfo.enableRayTracing) {
        winInfo.swapchainCreateInfo.imageUsage |= vk::ImageUsageFlagBits::eStorage;
    }
    auto window = instance->makeWindow(winInfo);
    auto ar = std::make_unique<AssetRegistry>(
        *instance,
        AssetRegistryCreateInfo{
            .enableRayTracing=instanceInfo.enableRayTracing
        }
    );
    auto sp = std::make_unique<ShadowPool>(*window, ShadowPoolCreateInfo{ .maxShadowMaps=200 });
    auto config{
        std::make_unique<DeferredRenderConfig>(
            *window,
            DeferredRenderCreateInfo{
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

void trc::pollEvents()
{
    vkb::pollEvents();
}

void trc::terminate()
{
    torchGlobalVulkanInstance.reset();

    RenderStageType::destroyAll();

    vkb::terminate();
}



trc::TorchStack::TorchStack(
    u_ptr<Instance> instance,
    u_ptr<Window> window,
    u_ptr<AssetRegistry> assetRegistry,
    u_ptr<ShadowPool> shadowPool,
    u_ptr<DeferredRenderConfig> renderConfig)
    :
    instance(std::move(instance)),
    window(std::move(window)),
    assetRegistry(std::move(assetRegistry)),
    shadowPool(std::move(shadowPool)),
    renderConfig(std::move(renderConfig))
{
}

trc::TorchStack::~TorchStack()
{
    if (window != nullptr) {
        window->getRenderer().waitForAllFrames();
    }
}

auto trc::TorchStack::makeDrawConfig(Scene& scene, Camera& camera) const -> DrawConfig
{
    return {
        .scene        = &scene,
        .camera       = &camera,
        .renderConfig = renderConfig.get(),
        .renderArea   = window->makeFullscreenRenderArea()
    };
}

void trc::TorchStack::drawFrame(const DrawConfig& draw)
{
    window->drawFrame(draw);
}
