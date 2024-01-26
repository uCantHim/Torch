#include "trc/Torch.h"

#include "trc/PipelineDefinitions.h"
#include "trc/RasterPlugin.h"
#include "trc/RasterTasks.h"
#include "trc/TorchRenderStages.h"
#include "trc/UpdatePass.h"
#include "trc/base/Logging.h"
#include "trc/base/event/EventHandler.h"
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



trc::TorchStack::TorchStack(
    const TorchStackCreateInfo& torchConfig,
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
    assetManager([&torchConfig]{
        fs::create_directories(torchConfig.assetStorageDir);
        return std::make_shared<FilesystemDataStorage>(torchConfig.assetStorageDir);
    }()),
    renderConfig(instance),
    shadowPool(std::make_shared<ShadowPool>(
        instance.getDevice(), window,
        ShadowPoolCreateInfo{ .maxShadowMaps=100 }
    )),
    frameSubmitter(instance.getDevice(), window)
{
    renderConfig.registerPlugin(std::make_shared<trc::RasterPlugin>(
        instance.getDevice(),
        window.getFrameCount(),
        trc::RasterPluginCreateInfo{
            .assetDescriptor             = trc::makeAssetDescriptor(
                instance,
                assetManager.getDeviceRegistry(),
                trc::AssetDescriptorCreateInfo{
                    .maxGeometries=10,
                    .maxTextures=10,
                    .maxFonts=1,
                }
            ),
            .shadowDescriptor            = shadowPool,
            .maxTransparentFragsPerPixel = 3,
            .enableRayTracing            = false,
            .mousePosGetter              = [&]{ return window.getMousePosition(); },
        }
    ));

    viewports = std::make_unique<FrameSpecific<ViewportConfig>>(
        renderConfig.makeViewportConfig(
            instance.getDevice(),
            makeRenderTarget(window),
            { 0, 0 },
            window.getSize()
        )
    );

    window.addCallbackAfterSwapchainRecreate([this](Swapchain& swapchain) {
        // Free viewport resources first
        viewports.reset();

        // Create new viewports
        viewports = std::make_unique<FrameSpecific<ViewportConfig>>(
            renderConfig.makeViewportConfig(
                instance.getDevice(),
                makeRenderTarget(swapchain),
                { 0, 0 },
                window.getSize()
            )
        );
    });
}

trc::TorchStack::~TorchStack()
{
    frameSubmitter.waitForAllFrames();
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
    auto frame = std::make_unique<Frame>();

    auto& currentViewport = **viewports;
    currentViewport.update(scene, camera);

    auto& drawGroup = frame->addViewport(currentViewport, scene);
    currentViewport.createTasks(scene, drawGroup.taskQueue);

    auto& pass = assetManager.getDeviceRegistry().getUpdatePass();
    drawGroup.taskQueue.spawnTask(
        resourceUpdateStage,
        makeTask([&pass](vk::CommandBuffer cmdBuf, TaskEnvironment& env) {
            pass.update(cmdBuf, *env.frame);
        })
    );

    frameSubmitter.renderFrame(std::move(frame));
}
