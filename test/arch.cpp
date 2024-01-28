#include <iostream>

#include <trc/AssetDescriptor.h>
#include <trc/RasterPlugin.h>
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
    trc::init();

    trc::Instance instance;
    trc::Window window{ instance };
    trc::RenderTarget renderTarget = trc::makeRenderTarget(window);

    trc::AssetManager assetManager{ std::make_shared<trc::NullDataStorage>() };
    auto assetDescriptor = trc::makeAssetDescriptor(
        instance,
        assetManager.getDeviceRegistry(),
        trc::AssetDescriptorCreateInfo{
            .maxGeometries=10,
            .maxTextures=10,
            .maxFonts=1,
        }
    );

    auto shadowDescriptor = std::make_shared<trc::ShadowPool>(
        instance.getDevice(), window, trc::ShadowPoolCreateInfo{ .maxShadowMaps=1 }
    );

    trc::RenderConfig renderConfig{ instance };
    trc::RasterPlugin rasterization{
        instance.getDevice(),
        window.getFrameCount(),
        trc::RasterPluginCreateInfo{
            .shadowDescriptor            = shadowDescriptor,
            .maxTransparentFragsPerPixel = 3,
        }
    };
    renderConfig.registerPlugin(std::make_shared<trc::RasterPlugin>(std::move(rasterization)));

    trc::SwapchainRenderer renderer{ instance.getDevice(), window };
    auto viewports = renderConfig.makeViewportConfig(
        instance.getDevice(),
        renderTarget,
        { 0, 0 }, window.getSize()
    );

    trc::SceneBase scene = renderConfig.makeScene();
    trc::Camera camera;

    // Draw
    auto frame = std::make_unique<trc::Frame>();
    auto& viewport = frame->addViewport(**viewports, scene);
    viewports.get()->update(instance.getDevice(), scene, camera);
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
