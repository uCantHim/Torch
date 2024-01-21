#include "trc/Torch.h"

#include "trc/PipelineDefinitions.h"
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
    shadowPool(instance.getDevice(), window, { .maxShadowMaps=100 }),
    renderConfig(instance, makeRenderTarget(window), { 0, 0 }, window.getSize()),
    frameSubmitter(instance.getDevice(), window)
{
    window.addCallbackAfterSwapchainRecreate([this](Swapchain&) {
        renderConfig.setRenderTarget(makeRenderTarget(window), { 0, 0 }, window.getSize());
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
    return shadowPool;
}

auto trc::TorchStack::getRenderConfig() -> RenderConfig&
{
    return renderConfig;
}

void trc::TorchStack::drawFrame(const Camera& camera, SceneBase& scene)
{
    auto frame = std::make_unique<Frame>(&instance.getDevice());

    //renderConfig.perFrameUpdate(camera, scene);
    frame->addViewport(renderConfig, scene);

    frameSubmitter.renderFrame(std::move(frame));
}
