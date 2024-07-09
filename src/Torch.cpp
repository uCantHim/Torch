#include "trc/Torch.h"

#include <cassert>

#include "trc/AssetPlugin.h"
#include "trc/RasterPlugin.h"
#include "trc/SwapchainPlugin.h"
#include "trc/base/Logging.h"
#include "trc/base/event/EventHandler.h"
#include "trc/ray_tracing/RayTracingPlugin.h"
#include "trc/util/FilesystemDataStorage.h"



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
    log::info << "GLFW initialized successfully";
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

auto trc::initFull(
    const TorchStackCreateInfo& torchConfig,
    const InstanceCreateInfo& instanceInfo,
    const WindowCreateInfo& windowInfo
    ) -> u_ptr<TorchStack>
{
    init();
    return std::make_unique<TorchStack>(torchConfig, instanceInfo, windowInfo);
}

auto trc::makeTorchRenderPipeline(
    const Instance& instance,
    const Swapchain& swapchain,
    const TorchRenderConfigCreateInfo& createInfo) -> u_ptr<RenderPipeline>
{
    RenderPipelineBuilder builder;
    builder.addPlugin([&swapchain](auto&) {
        return std::make_unique<SwapchainPlugin>(swapchain);
    });
    builder.addPlugin([&createInfo](PluginBuildContext&) {
        return std::make_unique<AssetPlugin>(
            createInfo.assetRegistry,
            createInfo.assetDescriptor
        );
    });
    builder.addPlugin([&createInfo](PluginBuildContext& ctx) {
        return std::make_unique<RasterPlugin>(
            ctx.device(),
            ctx.numViewports(),
            trc::RasterPluginCreateInfo{
                .shadowDescriptor            = createInfo.shadowDescriptor,
                .maxTransparentFragsPerPixel = 3,
            }
        );
    });
    if (createInfo.enableRayTracing && instance.hasRayTracing())
    {
        builder.addPlugin([&createInfo](PluginBuildContext& ctx) -> u_ptr<RenderPlugin> {
            return std::make_unique<RayTracingPlugin>(
                ctx.instance(),
                ctx.numViewports(),
                createInfo.maxRayGeometries
            );
        });
    }

    return builder.compile(RenderPipelineCreateInfo{
        instance,
        makeRenderTarget(swapchain),
        1,  // maximum number of viewports on the render target
    });
}


trc::TorchStack::TorchStack(
    const TorchStackCreateInfo& torchConfig,
    const InstanceCreateInfo& instanceInfo,
    const WindowCreateInfo& windowInfo,
    const AssetDescriptorCreateInfo& assetDescriptorInfo)
    :
    instance(instanceInfo),
    window(instance, [&] {
        auto winInfo = windowInfo;
        if (instanceInfo.enableRayTracing) {
            winInfo.swapchainCreateInfo.imageUsage |= vk::ImageUsageFlagBits::eStorage;
        }
        return winInfo;
    }()),
    assetManager([&torchConfig]{
        fs::create_directories(torchConfig.assetStorageDir);
        return std::make_shared<FilesystemDataStorage>(torchConfig.assetStorageDir);
    }()),
    assetDescriptor(trc::makeAssetDescriptor(
        instance,
        assetManager.getDeviceRegistry(),
        assetDescriptorInfo
    )),
    shadowPool(std::make_shared<ShadowPool>(
        instance.getDevice(),
        window,
        ShadowPoolCreateInfo{ .maxShadowMaps=100 }
    )),
    renderPipeline(makeTorchRenderPipeline(
        instance,
        window,
        TorchRenderConfigCreateInfo{
            .assetRegistry=assetManager.getDeviceRegistry(),
            .assetDescriptor=assetDescriptor,
            .shadowDescriptor=shadowPool,
            .enableRayTracing=instanceInfo.enableRayTracing && instance.hasRayTracing()
        }
    )),
    defaultCamera{},
    defaultScene{},
    viewport(renderPipeline->makeViewport(
        RenderArea{ { 0, 0 }, window.getSize() },
        defaultCamera,
        defaultScene
    )),
    renderer(instance.getDevice(), window)
{
    window.addCallbackAfterSwapchainRecreate([this](Swapchain&) {
        renderPipeline.reset();
        viewport.reset();

        // Recreate the entire render pipeline on swapchain recreate.
        // This may not be optimal.
        renderPipeline = makeTorchRenderPipeline(
            instance,
            window,
            TorchRenderConfigCreateInfo{
                .assetRegistry=assetManager.getDeviceRegistry(),
                .assetDescriptor=assetDescriptor,
                .shadowDescriptor=shadowPool,
                .enableRayTracing=instance.hasRayTracing()
            }
        );
        viewport = renderPipeline->makeViewport(
            RenderArea{ { 0, 0 }, window.getSize() },
            defaultCamera,
            defaultScene
        );
    });
}

trc::TorchStack::~TorchStack()
{
    renderer.waitForAllFrames();
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
    return *shadowPool;
}

auto trc::TorchStack::getRenderPipeline() -> RenderPipeline&
{
    assert(renderPipeline != nullptr);
    return *renderPipeline;
}

void trc::TorchStack::drawFrame(Camera& camera, SceneBase& scene)
{
    viewport.setCamera(camera);
    viewport.setScene(scene);

    // Create a frame and draw to it
    auto frame = renderPipeline->draw();
    renderer.renderFrameAndPresent(std::move(frame), window);
}

void trc::TorchStack::waitForAllFrames(ui64 timeoutNs)
{
    renderer.waitForAllFrames(timeoutNs);
}
