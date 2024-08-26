#include "trc/Torch.h"

#include <cassert>

#include <trc_util/Assert.h>

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
    Instance& instance,
    Window& window,
    const TorchPipelineCreateInfo& createInfo,
    const vk::ArrayProxy<TorchPipelinePluginBuilder>& plugins)
    -> u_ptr<RenderPipeline>
{
    assert_arg(createInfo.maxViewports > 0);

    RenderPipelineBuilder builder;
    builder.addPlugin(buildSwapchainPlugin(window));
    builder.addPlugin([&createInfo](PluginBuildContext&) {
        return std::make_unique<AssetPlugin>(
            createInfo.assetRegistry,
            createInfo.assetDescriptor
        );
    });
    builder.addPlugin(buildRasterPlugin(
        trc::RasterPluginCreateInfo{
            .maxShadowMaps=createInfo.maxShadowMaps,
            .maxTransparentFragsPerPixel=createInfo.maxTransparentFragsPerPixel,
        }
    ));
    if (createInfo.enableRayTracing && instance.hasRayTracing())
    {
        builder.addPlugin(buildRayTracingPlugin(
            trc::RayTracingPluginCreateInfo{
                .maxTlasInstances=createInfo.maxRayGeometries
            }
        ));
    }

    // Add user-supplied plugins
    for (auto& plugin : plugins) {
        builder.addPlugin(plugin(window));
    }

    return builder.build(RenderPipelineCreateInfo{
        instance,
        makeRenderTarget(window),
        createInfo.maxViewports,
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
    renderPipeline(makeTorchRenderPipeline(
        instance,
        window,
        TorchPipelineCreateInfo{
            .assetRegistry=assetManager.getDeviceRegistry(),
            .assetDescriptor=assetDescriptor,
            .maxShadowMaps=kDefaultMaxShadowMaps,
            .maxTransparentFragsPerPixel=kDefaultMaxTransparentFrags,
            .enableRayTracing=instanceInfo.enableRayTracing && instance.hasRayTracing()
        },
        torchConfig.plugins
    )),
    renderer(instance.getDevice(), window)
{
    window.addCallbackAfterSwapchainRecreate([this](Swapchain& sc) {
        if (&sc != &window) {
            return;
        }

        renderer.waitForAllFrames();
        renderPipeline->changeRenderTarget(makeRenderTarget(window));
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

auto trc::TorchStack::getRenderPipeline() -> RenderPipeline&
{
    assert(renderPipeline != nullptr);
    return *renderPipeline;
}

auto trc::TorchStack::makeViewport(
    const RenderArea& area,
    const s_ptr<Camera>& camera,
    const s_ptr<SceneBase>& scene)
    -> ViewportHandle
{
    return renderPipeline->makeViewport(area, camera, scene);
}

auto trc::TorchStack::makeFullscreenViewport(
    const s_ptr<Camera>& camera,
    const s_ptr<SceneBase>& scene)
    -> ViewportHandle
{
    auto vp = makeViewport(RenderArea{ {0, 0}, window.getSize() }, camera, scene);
    vp->onRenderTargetUpdate([](auto&&, const RenderTarget& target) {
        return RenderArea{ {0, 0}, target.getSize() };
    });
    return vp;
}

void trc::TorchStack::drawFrame(const vk::ArrayProxy<ViewportHandle>& viewports)
{
    auto frame = renderPipeline->draw(viewports);
    renderer.renderFrameAndPresent(std::move(frame), window);
}

void trc::TorchStack::waitForAllFrames(ui64 timeoutNs)
{
    renderer.waitForAllFrames(timeoutNs);
}
