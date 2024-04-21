#include "trc/Torch.h"

#include "trc/AssetPlugin.h"
#include "trc/PipelineDefinitions.h"
#include "trc/RasterPlugin.h"
#include "trc/RasterTasks.h"
#include "trc/TorchRenderStages.h"
#include "trc/UpdatePass.h"
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

auto trc::makeTorchRenderConfig(
    const Instance& instance,
    ui32 maxViewports,
    const TorchRenderConfigCreateInfo& createInfo) -> RenderConfig
{
    const Device& device{ instance.getDevice() };

    RenderConfig renderConfig{ instance };
    renderConfig.registerPlugin(std::make_shared<AssetPlugin>(
        createInfo.assetRegistry,
        createInfo.assetDescriptor
    ));
    renderConfig.registerPlugin(std::make_shared<RasterPlugin>(
        device,
        maxViewports,
        trc::RasterPluginCreateInfo{
            .shadowDescriptor            = createInfo.shadowDescriptor,
            .maxTransparentFragsPerPixel = 3,
        }
    ));
    if (createInfo.enableRayTracing && instance.hasRayTracing())
    {
        renderConfig.registerPlugin(std::make_shared<RayTracingPlugin>(
                instance,
                renderConfig.getResourceConfig(),
                maxViewports,
                createInfo.maxRayGeometries
            )
        );
    }

    return renderConfig;
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
    renderConfig(makeTorchRenderConfig(
        instance,
        window.getFrameCount(),
        TorchRenderConfigCreateInfo{
            .assetRegistry=assetManager.getDeviceRegistry(),
            .assetDescriptor=assetDescriptor,
            .shadowDescriptor=shadowPool,
            .enableRayTracing=instanceInfo.enableRayTracing && instance.hasRayTracing()
        }
    )),
    renderer(instance.getDevice(), window),
    viewports(renderConfig.makeViewports(
        instance.getDevice(),
        makeRenderTarget(window),
        { 0, 0 },
        window.getSize()
    ))
{
    window.addCallbackAfterSwapchainRecreate([this](Swapchain& swapchain) {
        // Free viewport resources first
        viewports = { swapchain };

        // Create new viewports
        viewports = renderConfig.makeViewports(
            instance.getDevice(),
            makeRenderTarget(swapchain),
            { 0, 0 },
            swapchain.getSize()
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

auto trc::TorchStack::getRenderConfig() -> RenderConfig&
{
    return renderConfig;
}

void trc::TorchStack::drawFrame(const Camera& camera, SceneBase& scene)
{
    // Create a frame and draw to it
    auto frame = std::make_unique<Frame>();

    auto& currentViewport = **viewports;
    currentViewport.update(instance.getDevice(), scene, camera);
    frame->addViewport(currentViewport, scene);

    renderer.renderFrameAndPresent(std::move(frame), window);
}

void trc::TorchStack::waitForAllFrames(ui64 timeoutNs)
{
    renderer.waitForAllFrames(timeoutNs);
}
