#include <iostream>

#include <trc/AssetDescriptor.h>
#include <trc/RasterPlugin.h>
#include <trc/RasterSceneModule.h>
#include <trc/RaySceneModule.h>
#include <trc/SwapchainPlugin.h>
#include <trc/Torch.h>
#include <trc/TorchRenderStages.h>
#include <trc/base/Barriers.h>
#include <trc/core/Renderer.h>
#include <trc/core/SceneBase.h>
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

    // Create a render pipeline
    auto pipeline = trc::RenderPipelineBuilder{}
        .addPlugin(trc::buildRasterPlugin({
            trc::RasterPluginCreateInfo{
                .maxShadowMaps = 1,
                .maxTransparentFragsPerPixel = 3,
            }
        }))
        .addPlugin(trc::buildSwapchainPlugin(window))
        .build({
            .instance=instance,
            .renderTarget=renderTarget,
            .maxViewports=1,
        });

    const auto myRenderStage = trc::RenderStage::make();
    pipeline->getRenderGraph().insert(myRenderStage);

    auto scene = std::make_shared<trc::Scene>();
    auto camera = std::make_shared<trc::Camera>();
    auto viewport = pipeline->makeViewport(
        trc::RenderArea{ { 0, 0 }, window.getSize() },
        camera,
        scene
    );

    // Draw
    trc::Renderer renderer{ instance.getDevice(), window };

    auto frame = pipeline->draw();
    frame->spawnTask(
        myRenderStage,
        [&window](vk::CommandBuffer, trc::DeviceExecutionContext&) {
            std::cout << "Hello Task!\n";
        }
    );

    renderer.renderFrameAndPresent(std::move(frame), window);
    renderer.waitForAllFrames();

    trc::terminate();

    return 0;
}
