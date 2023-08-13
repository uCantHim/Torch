#include <iostream>

#include <trc/Torch.h>
#include <trc/ImguiIntegration.h>

namespace trc {
    namespace imgui = experimental::imgui;
}
namespace ig = ImGui;

int main()
{
    {
        auto torch = trc::initFull();
        trc::Scene scene;
        trc::Camera camera;

        auto imgui = trc::imgui::initImgui(torch->getWindow(), torch->getRenderConfig().getRenderGraph());

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
