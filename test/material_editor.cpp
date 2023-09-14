#include <trc/Torch.h>
#include <trc/material/CommonShaderFunctions.h>

#include <GraphScene.h>
#include <Controls.h>
#include <MaterialEditorGui.h>
#include <MaterialEditorRenderConfig.h>

int main()
{
    trc::init();

    trc::Instance instance{ { .enableRayTracing=false } };
    trc::Window window{ instance };
    trc::RenderTarget renderTarget = trc::makeRenderTarget(window);

    MaterialEditorRenderConfig config{
        renderTarget,
        window,
        MaterialEditorRenderingInfo{
            .renderTargetBarrier=vk::ImageMemoryBarrier2{
                vk::PipelineStageFlagBits2::eAllCommands,
                vk::AccessFlagBits2::eHostWrite | vk::AccessFlagBits2::eHostRead,
                {}, {},  // dst flags; will be set by the renderer
                vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal
            },
            .finalLayout=vk::ImageLayout::ePresentSrcKHR
        }
    };
    trc::on<trc::SwapchainRecreateEvent>([&](auto&&) {
        renderTarget = trc::makeRenderTarget(window);
        config.setRenderTarget(renderTarget);
        config.setViewport({ 0, 0 }, renderTarget.getSize());
    });

    trc::SceneBase scene;
    trc::Camera camera;
    camera.makeOrthogonal(0.0f, 1.0f, 0.0f, 1.0f, -10.0f, 10.0f);

    GraphScene materialGraph;
    materialGraph.makeNode(std::make_shared<trc::Mix<3, float>>());

    MaterialEditorGui gui{ window, std::make_shared<GraphManipulator>(materialGraph) };
    MaterialEditorControls controls{ window, gui, camera };

    while (window.isOpen() && !window.isPressed(trc::Key::escape))
    {
        trc::pollEvents();

        controls.update(materialGraph);

        // Generate renderable data from graph
        const auto renderData = buildRenderData(materialGraph);

        // Draw a frame
        gui.drawGui();

        config.update(camera, renderData);
        window.drawFrame(trc::DrawConfig{
            .scene=scene,
            .renderConfig=config,
        });
    }

    trc::terminate();

    return 0;
}
