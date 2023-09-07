#include <iostream>

#include <trc/Torch.h>
#include <trc/ImguiIntegration.h>

namespace ig = ImGui;

int main()
{
    {
        auto torch = trc::initFull();
        trc::Scene scene;
        trc::Camera camera;

        auto& renderGraph = torch->getRenderConfig().getRenderGraph();
        renderGraph.after(trc::postProcessingRenderStage, trc::imgui::imguiRenderStage);
        auto imgui = trc::imgui::initImgui(torch->getWindow(), renderGraph);

        while (torch->getWindow().isOpen())
        {
            trc::pollEvents();

            trc::imgui::beginImguiFrame();
            ig::Begin("Window :D");
            ig::Text("Hello World!");
            ig::End();

            torch->drawFrame(camera, scene);
        }
    }

    trc::terminate();

    std::cout << "Done.\n";
    return 0;
}
