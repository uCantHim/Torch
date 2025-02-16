#include "graphics/Graphics.h"

#include <trc/ImageClear.h>
#include <trc/ImguiIntegration.h>



GraphicsStack::GraphicsStack()
    :
    instance([]{
        trc::init();
        return trc::Instance{};
    }())
{
}

GraphicsStack::~GraphicsStack() noexcept
{
    trc::terminate();
}

auto GraphicsStack::getDevice() -> trc::Device&
{
    return instance.getDevice();
}

auto GraphicsStack::getInstance() -> trc::Instance&
{
    return instance;
}

auto GraphicsStack::makeWindow(trc::AssetRegistry& assets) -> u_ptr<WindowRenderer>
{
    auto window = std::make_unique<trc::Window>(
        instance,
        trc::WindowCreateInfo{
            .swapchainCreateInfo{
                .imageUsage = vk::ImageUsageFlagBits::eTransferDst,
            }
        }
    );
    auto renderer = std::make_unique<trc::Renderer>(instance.getDevice(), *window);
    auto pipeline = trc::makeTorchRenderPipeline(
        window->getInstance(),
        *window,
        trc::TorchPipelineCreateInfo{
            .assetRegistry=assets,
            .assetDescriptorCreateInfo={},
        },
        { /* Additional render plugins */
            trc::imgui::buildImguiRenderPlugin,
            [](auto&&) -> trc::PluginBuilder {
                return trc::buildRenderTargetImageClearPlugin(
                    vk::ClearColorValue{ kClearColor.r, kClearColor.g, kClearColor.b, 1.0f }
                );
            },
        }
    );

    window->addCallbackAfterSwapchainRecreate(
        [renderer=renderer.get(), pipeline=pipeline.get()](trc::Swapchain& sc)
        {
            renderer->waitForAllFrames();
            pipeline->changeRenderTarget(makeRenderTarget(sc));
        }
    );

    return std::make_unique<WindowRenderer>(WindowRenderer{
        std::move(window),
        std::move(renderer),
        std::move(pipeline),
    });
}



auto WindowRenderer::getWindow() -> trc::Window&
{
    return *window;
}

auto WindowRenderer::getRenderPipeline() -> trc::RenderPipeline&
{
    return *renderPipeline;
}

auto WindowRenderer::makeViewport(
    const trc::RenderArea& extent,
    s_ptr<trc::Camera> camera,
    s_ptr<trc::Scene> scene) -> trc::ViewportHandle
{
    return renderPipeline->makeViewport(extent, camera, scene);
}

auto WindowRenderer::makeFrame() -> u_ptr<trc::Frame>
{
    return renderPipeline->makeFrame();
}

void WindowRenderer::submitFrame(u_ptr<trc::Frame> frame)
{
    renderer->renderFrameAndPresent(std::move(frame), *window);
}
