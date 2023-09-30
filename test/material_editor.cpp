#include <trc/Torch.h>
#include <trc/material/CommonShaderFunctions.h>
#include <trc/util/NullDataStorage.h>

#include <Controls.h>
#include <Font.h>
#include <GraphScene.h>
#include <GraphSerializer.h>
#include <MaterialEditorCommands.h>
#include <MaterialEditorGui.h>
#include <MaterialEditorRenderConfig.h>
#include <MaterialPreview.h>

int main()
{
    trc::init();

    trc::Instance instance{ { .enableRayTracing=false } };
    trc::Window window{ instance };
    trc::RenderTarget renderTarget = trc::makeRenderTarget(window);

    const auto fontPath = TRC_TEST_FONT_DIR"/gil.ttf";
    const auto monoFontPath = TRC_TEST_FONT_DIR"/hack_mono.ttf";
    setGlobalTextFont(Font{ window.getDevice(), trc::loadFont(fontPath, 64) });
    setGlobalMonoFont(Font{ window.getDevice(), trc::loadFont(monoFontPath, 64) });

    // -------------------------------------------------------------------------
    // Material preview viewport:

    MaterialPreview preview{ instance, renderTarget };

    preview.setViewport({ window.getSize().x - 350 - 20, 20 }, { 350, 350 });
    trc::on<trc::SwapchainRecreateEvent>([&](auto&&) {
        preview.setRenderTarget(renderTarget);
        preview.setViewport({ renderTarget.getSize().x - 350 - 20, 20 }, { 350, 350 });
    });

    // -------------------------------------------------------------------------
    // Material graph viewport:

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

    // -------------------------------------------------------------------------
    // Material graph data:

    GraphScene materialGraph;
    if (std::ifstream file = std::ifstream(".matedit_save", std::ios::binary))
    {
        if (auto graph = parseGraph(file)) {
            materialGraph = std::move(*graph);
        }
    }

    // The graph must contain an output node. Create one if none exists.
    if (materialGraph.graph.outputNode == NodeID::NONE) {
        materialGraph.graph.outputNode = materialGraph.makeNode(getOutputNode());
    }

    MaterialEditorCommands commands{ materialGraph, preview };
    MaterialEditorGui gui{ window, commands };
    MaterialEditorControls controls{ window, gui, camera, { .initialZoomLevel=5 } };

    while (window.isOpen() && !window.isPressed(trc::Key::escape))
    {
        trc::pollEvents();

        controls.update(materialGraph, commands);
        updateOutputValues(materialGraph.interaction, materialGraph.graph);

        // Generate renderable data from graph
        const auto renderData = buildRenderData(materialGraph);

        // Draw a frame
        gui.drawGui();

        config.update(camera, renderData);
        preview.update();
        window.drawFrame({
            trc::DrawConfig{
                .scene=scene,
                .renderConfig=config,
            },
            preview.getDrawConfig()
        });
    }

    destroyGlobalFonts();
    trc::terminate();

    return 0;
}
