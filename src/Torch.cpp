#include "Torch.h"

#include "UpdatePass.h"
#include "TorchResources.h"
#include "ui/torch/GuiIntegration.h"
#ifdef TRC_USE_IMGUI
#include "experimental/ImguiIntegration.h"
#endif
#include "ray_tracing/RayTracing.h"



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
    if (torchGlobalVulkanInstance == nullptr) {
        init();
    }

    return *torchGlobalVulkanInstance;
}

auto trc::makeTorchRenderGraph() -> RenderGraph
{
    // Create render graph
    auto graph = makeDeferredRenderGraph();

    // Ray tracing stages
    graph.after(finalLightingRenderStage, rt::rayTracingRenderStage);
    graph.after(rt::rayTracingRenderStage, rt::finalCompositingStage);
    graph.require(rt::rayTracingRenderStage, resourceUpdateStage);
    graph.require(rt::finalCompositingStage, finalLightingRenderStage);
    graph.require(rt::finalCompositingStage, rt::rayTracingRenderStage);

    graph.after(rt::finalCompositingStage, guiRenderStage);
#ifdef TRC_USE_IMGUI
    graph.after(guiRenderStage, experimental::imgui::imguiRenderStage);
#endif

    return graph;
}

auto trc::initFull(
    const InstanceCreateInfo& instanceInfo,
    const WindowCreateInfo& windowInfo
    ) -> u_ptr<TorchStack>
{
    return std::make_unique<TorchStack>(instanceInfo, windowInfo);
}

void trc::pollEvents()
{
    vkb::pollEvents();
}

void trc::terminate()
{
    torchGlobalVulkanInstance.reset();

    vkb::terminate();
}



trc::TorchStack::TorchStack(
    const InstanceCreateInfo& instanceInfo,
    const WindowCreateInfo& windowInfo)
    :
    instance(instanceInfo, *getVulkanInstance()),
    window(instance, [&] {
        auto winInfo = windowInfo;
        if (instanceInfo.enableRayTracing) {
            winInfo.swapchainCreateInfo.imageUsage |= vk::ImageUsageFlagBits::eStorage;
        }
        return winInfo;
    }()),
    assetManager(
        instance,
        AssetRegistryCreateInfo{
            .enableRayTracing=instanceInfo.enableRayTracing && instance.hasRayTracing()
        }
    ),
    shadowPool(window, ShadowPoolCreateInfo{ .maxShadowMaps=200 }),
    swapchainRenderTarget(makeRenderTarget(window)),
    renderConfig(
        window,
        TorchRenderConfigCreateInfo{
            makeTorchRenderGraph(),
            swapchainRenderTarget,
            &assetManager.getDeviceRegistry(),
            &shadowPool,
            3  // max transparent frags
        }
    ),
    swapchainRecreateListener(
        vkb::on<vkb::SwapchainRecreateEvent>(
        [this]
            (const vkb::SwapchainRecreateEvent& e)
        {
            if (e.swapchain == &window)
            {
                const uvec2 newSize = e.swapchain->getSize();
                swapchainRenderTarget = makeRenderTarget(*e.swapchain);
                renderConfig.setRenderTarget(swapchainRenderTarget);
                renderConfig.setViewport({ 0, 0 }, newSize);

            }
        })
    )
{
    renderConfig.setViewport({ 0, 0 }, window.getSize());
}

trc::TorchStack::~TorchStack()
{
    window.getRenderer().waitForAllFrames();
}

auto trc::TorchStack::getDevice() -> vkb::Device&
{
    return instance.getDevice();
}

auto trc::TorchStack::getInstance() -> Instance&
{
    return instance;
}

auto trc::TorchStack::getWindow() -> Window&
{
    return window;
}

auto trc::TorchStack::getAssetManager() -> AssetManager&
{
    return assetManager;
}

auto trc::TorchStack::getShadowPool() -> ShadowPool&
{
    return shadowPool;
}

auto trc::TorchStack::getRenderTarget() -> RenderTarget&
{
    return swapchainRenderTarget;
}

auto trc::TorchStack::getRenderConfig() -> TorchRenderConfig&
{
    return renderConfig;
}

auto trc::TorchStack::makeDrawConfig(Scene& scene, Camera& camera) -> DrawConfig
{
    return {
        .scene        = &scene,
        .camera       = &camera,
        .renderConfig = &renderConfig
    };
}

void trc::TorchStack::drawFrame(const DrawConfig& draw)
{
    window.drawFrame(draw);
}
