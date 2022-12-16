#include "trc/Torch.h"

#include "trc/DrawablePipelines.h"
#include "trc/ParticlePipelines.h"
#include "trc/PipelineDefinitions.h"
#include "trc/RasterPipelines.h"
#include "trc/TextPipelines.h"
#include "trc/TorchRenderStages.h"
#include "trc/UpdatePass.h"
#include "trc/base/Logging.h"
#include "trc/ui/torch/GuiIntegration.h"
#ifdef TRC_USE_IMGUI
#include "trc/experimental/ImguiIntegration.h"
#endif



static inline bool isInitialized{ false };

void trc::init(const TorchInitInfo& info)
{
    if (isInitialized) return;
    isInitialized = true;

    if (info.startEventThread) {
        EventThread::start();
    }

    // Init GLFW first
    if (glfwInit() == GLFW_FALSE)
    {
        const char* errorMsg{ nullptr };
        glfwGetError(&errorMsg);
        throw std::runtime_error("Initialization of GLFW failed: " + std::string(errorMsg));
    }
    log::info << "GLFW initialized successfully\n";

    // Init pipelines
    pipelines::initDrawablePipelines({
        .shaderLoader=internal::getShaderLoader(),
        .drawablePushConstantDefaultValue=DrawablePushConstants{}
    });
    pipelines::initRasterPipelines({ internal::getShaderLoader() });
    pipelines::text::initTextPipelines({ internal::getShaderLoader() });
    pipelines::particle::initParticlePipelines({ internal::getShaderLoader() });
}

void trc::pollEvents()
{
    glfwPollEvents();
}

void trc::terminate()
{
    EventThread::terminate();
    glfwTerminate();

    isInitialized = false;
}

auto trc::makeTorchRenderGraph() -> RenderGraph
{
    // Create render graph
    auto graph = makeDeferredRenderGraph();

    // Ray tracing stages
    graph.after(finalLightingRenderStage, rayTracingRenderStage);
    graph.require(rayTracingRenderStage, resourceUpdateStage);
    graph.require(rayTracingRenderStage, finalLightingRenderStage);

    graph.after(rayTracingRenderStage, guiRenderStage);
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
    init();
    return std::make_unique<TorchStack>(instanceInfo, windowInfo);
}



trc::TorchStack::TorchStack(
    const InstanceCreateInfo& instanceInfo,
    const WindowCreateInfo& windowInfo)
    :
    instance(instanceInfo),
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
            .renderGraph                 = makeTorchRenderGraph(),
            .target                      = swapchainRenderTarget,
            .assetRegistry               = &assetManager.getDeviceRegistry(),
            .shadowPool                  = &shadowPool,
            .maxTransparentFragsPerPixel = 3,
            .enableRayTracing            = instanceInfo.enableRayTracing
        }
    ),
    swapchainRecreateListener(
        on<SwapchainRecreateEvent>(
        [this]
            (const SwapchainRecreateEvent& e)
        {
            if (e.swapchain == &window)
            {
                const uvec2 newSize = e.swapchain->getSize();
                swapchainRenderTarget = makeRenderTarget(*e.swapchain);
                renderConfig.setRenderTarget(swapchainRenderTarget);
                renderConfig.setViewport({ 0, 0 }, newSize);

            }
        }).makeUnique()
    )
{
    renderConfig.setViewport({ 0, 0 }, window.getSize());
}

trc::TorchStack::~TorchStack()
{
    window.getRenderer().waitForAllFrames();
}

auto trc::TorchStack::getDevice() -> Device&
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

void trc::TorchStack::drawFrame(const Camera& camera, const Scene& scene)
{
    renderConfig.perFrameUpdate(camera, scene);
    window.drawFrame(DrawConfig{ .scene=scene, .renderConfig=renderConfig });
}
