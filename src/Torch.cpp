#include "Torch.h"

#include <IL/il.h>

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

auto trc::initFull(
    const InstanceCreateInfo& instanceInfo,
    const WindowCreateInfo& windowInfo
    ) -> TorchStack
{
    // Create render graph
    auto graph = makeDeferredRenderGraph();

    graph.first(resourceUpdateStage);

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

    auto target = std::make_unique<RenderTarget>(makeRenderTarget(*window));
    auto config{
        std::make_unique<DeferredRenderConfig>(
            *window,
            DeferredRenderCreateInfo{
                std::move(graph),
                *target,
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
        std::move(target),
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

    vkb::terminate();
}



trc::TorchStack::TorchStack(
    u_ptr<Instance> _instance,
    u_ptr<Window> _window,
    u_ptr<AssetRegistry> _assetRegistry,
    u_ptr<ShadowPool> _shadowPool,
    u_ptr<RenderTarget> _swapchainRenderTarget,
    u_ptr<DeferredRenderConfig> _renderConfig)
    :
    instance(std::move(_instance)),
    window(std::move(_window)),
    assetRegistry(std::move(_assetRegistry)),
    shadowPool(std::move(_shadowPool)),
    swapchainRenderTarget(std::move(_swapchainRenderTarget)),
    renderConfig(std::move(_renderConfig)),
    swapchainRecreateListener(
        vkb::on<vkb::SwapchainRecreateEvent>(
        [window=window.get(), rc=renderConfig.get(), target=swapchainRenderTarget.get()]
            (const vkb::SwapchainRecreateEvent& e)
        {
            if (e.swapchain == window)
            {
                const uvec2 newSize = e.swapchain->getSize();
                *target = makeRenderTarget(*e.swapchain);
                rc->setRenderTarget(*target);
                rc->setViewport({ 0, 0 }, newSize);

            }
        })
    )
{
    renderConfig->setViewport({ 0, 0 }, window->getSize());
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
        .renderConfig = renderConfig.get()
    };
}

void trc::TorchStack::drawFrame(const DrawConfig& draw)
{
    window->drawFrame(draw);
}
