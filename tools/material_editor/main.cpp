#include <trc/ImguiIntegration.h>
#include <trc/SwapchainPlugin.h>
#include <trc/Torch.h>
#include <trc/base/event/Event.h>
#include <trc/material/shader/CommonShaderFunctions.h>
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
    trc::Renderer renderer{ instance.getDevice(), window };

    const auto fontPath = TRC_TEST_FONT_DIR"/gil.ttf";
    const auto monoFontPath = TRC_TEST_FONT_DIR"/hack_mono.ttf";
    setGlobalTextFont(Font{ window.getDevice(), trc::loadFont(fontPath, 64) });
    setGlobalMonoFont(Font{ window.getDevice(), trc::loadFont(monoFontPath, 64) });

    auto materialEditorPipeline = trc::buildRenderPipeline()
        .addPlugin([](trc::PluginBuildContext& ctx) -> u_ptr<trc::RenderPlugin> {
            return std::make_unique<MaterialEditorRenderPlugin>(
                ctx.device(),
                ctx.renderTarget()
            );
        })
        .addPlugin(trc::imgui::buildImguiRenderPlugin(window))
        .addPlugin(trc::buildSwapchainPlugin(window))
        .build({
            .instance=instance,
            .renderTarget=trc::makeRenderTarget(window),
            .maxViewports=1,
        });

    // -------------------------------------------------------------------------
    // Material preview viewport:

    MaterialPreview preview{
        window,
        trc::RenderArea{ { window.getSize().x - 350 - 20, 20 }, { 350, 350 } }
    };
    trc::on<trc::SwapchainResizeEvent>([&](auto&&) {
        preview.setViewport({ window.getSize().x - 350 - 20, 20 }, { 350, 350 });
    });

    // -------------------------------------------------------------------------
    // Material graph viewport:

    auto scene = std::make_shared<trc::Scene>();
    auto camera = std::make_shared<trc::Camera>();
    camera->makeOrthogonal(0.0f, 1.0f, 0.0f, 1.0f, -10.0f, 10.0f);

    auto viewport = materialEditorPipeline->makeViewport({ {}, window.getSize() }, camera, scene);
    trc::on<trc::SwapchainResizeEvent>([&](auto&&) {
        materialEditorPipeline->changeRenderTarget(trc::makeRenderTarget(window));
        viewport->resize({ {}, window.getSize() });
    });

    // -------------------------------------------------------------------------
    // Material graph:

    scene->registerModule(std::make_unique<GraphScene>());
    GraphScene& materialGraph = scene->getModule<GraphScene>();

    if (std::ifstream file{ ".matedit_save", std::ios::binary })
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
    MaterialEditorControls controls{ window, gui, *camera, { .initialZoomLevel=5 } };

    while (window.isOpen() && !window.isPressed(trc::Key::escape))
    {
        trc::pollEvents();

        controls.update(materialGraph, commands);

        // Execute ImGui commands
        gui.drawGui();

        // Draw a frame
        auto frame = materialEditorPipeline->makeFrame();
        preview.draw(*frame);
        materialEditorPipeline->drawAllViewports(*frame);
        renderer.renderFrameAndPresent(std::move(frame), window);
    }

    destroyGlobalFonts();
    trc::terminate();

    return 0;
}
