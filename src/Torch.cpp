#include "trc/Torch.h"

#include "trc/PipelineDefinitions.h"
#include "trc/TorchImplementation.h"
#include "trc/TorchRenderStages.h"
#include "trc/UpdatePass.h"
#include "trc/base/Logging.h"
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
    swapchainRenderTarget(makeRenderTarget(window)),
    renderConfig(
        instance,
        TorchRenderConfigCreateInfo{
            .target                      = swapchainRenderTarget,
            .assetRegistry               = &assetManager.getDeviceRegistry(),
            .assetDescriptor             = makeDefaultAssetModules(
                instance,
                assetManager.getDeviceRegistry(),
                AssetDescriptorCreateInfo{
                    // TODO: Put these settings into a global configuration object
                    .maxGeometries = 5000,
                    .maxTextures = 2000,
                    .maxFonts = 50,
                }
            ),
            .maxTransparentFragsPerPixel = 3,
            .enableRayTracing            = instanceInfo.enableRayTracing,
            .mousePosGetter              = [&]{ return window.getMousePositionLowerLeft(); },
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
    return renderConfig.getShadowPool();
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
