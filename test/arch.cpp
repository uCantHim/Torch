#include <iostream>

#include <trc/RasterSceneModule.h>
#include <trc/RaySceneModule.h>
#include <trc/Torch.h>
#include <trc/TorchRenderStages.h>
#include <trc/base/Barriers.h>
#include <trc/core/SceneBase.h>
#include <trc/core/SwapchainRenderer.h>
#include <trc/util/NullDataStorage.h>

#include <trc_util/Assert.h>

int main()
{
    assert_arg(false == true);

    trc::init();

    trc::Instance instance;
    trc::Window window{ instance };
    trc::RenderTarget renderTarget = trc::makeRenderTarget(window);

    trc::AssetManager assetManager{ std::make_shared<trc::NullDataStorage>() };
    trc::TorchRenderConfig config{
        instance,
        trc::TorchRenderConfigCreateInfo{
            renderTarget,
            &assetManager.getDeviceRegistry(),
            trc::makeDefaultAssetModules(
                instance, assetManager.getDeviceRegistry(),
                { .maxGeometries=10, .maxTextures=10, .maxFonts=1 }),
            2,
            false
        }
    };

    trc::SceneBase scene;
    trc::Camera camera;

    scene.registerModule(std::make_unique<trc::RasterSceneModule>());
    scene.registerModule(std::make_unique<trc::RaySceneModule>());

    trc::SwapchainRenderer renderer{ instance.getDevice(), window };

    auto frame = std::make_unique<trc::Frame>(&instance.getDevice());
    auto& viewport = frame->addViewport(config, scene);
    config.perFrameUpdate(camera, scene);
    viewport.taskQueue.spawnTask(
        trc::gBufferRenderStage,
        trc::makeTask([&window](vk::CommandBuffer cmdBuf, trc::TaskEnvironment& env) {
            std::cout << "Hello Task!\n";
            auto image = window.getImage(window.getCurrentFrame());
            trc::barrier(cmdBuf, vk::ImageMemoryBarrier2{
                vk::PipelineStageFlagBits2::eBottomOfPipe,
                vk::AccessFlagBits2::eNone,
                vk::PipelineStageFlagBits2::eTopOfPipe,
                vk::AccessFlagBits2::eNone,
                vk::ImageLayout::eUndefined,
                vk::ImageLayout::ePresentSrcKHR,
                VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
                image,
                vk::ImageSubresourceRange{ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 }
            });
        })
    );
    renderer.renderFrame(std::move(frame));

    renderer.waitForAllFrames();

    trc::terminate();

    return 0;
}
