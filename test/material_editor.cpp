#include <trc/Torch.h>

#include <GraphScene.h>
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

    trc::SceneBase scene;
    trc::Camera camera;
    camera.makeOrthogonal(-1.0f, 1.0f, -1.0f, 1.0f, -10.0f, 10.0f);

    GraphScene materialGraph;
    materialGraph.makeNode();

    MaterialEditorGui gui{ window, std::make_shared<GraphManipulator>(materialGraph) };

    while (window.isOpen() && !window.isPressed(trc::Key::escape))
    {
        trc::pollEvents();

        // Generate renderable data from graph
        const auto renderData = buildRenderData(materialGraph.graph, materialGraph.layout);

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
