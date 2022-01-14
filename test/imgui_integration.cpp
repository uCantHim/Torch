#include <iostream>

#include <trc/Torch.h>
#include <trc/experimental/ImguiIntegration.h>
#include <trc/experimental/ImguiUtils.h>
namespace ig = ImGui;

namespace trc {
    namespace imgui = experimental::imgui;
}

int main()
{
    {
        auto torch = trc::initFull();
        trc::Scene scene;
        trc::Camera camera;

        auto imgui = trc::imgui::initImgui(torch->getWindow(), torch->getRenderConfig().getLayout());

        while (torch->getWindow().isOpen())
        {
            vkb::pollEvents();

            trc::imgui::beginImguiFrame();
            ig::Begin("Window :D");
            ig::Text("Hello World!");
            ig::End();

            torch->drawFrame(torch->makeDrawConfig(scene, camera));
        }
    }

    trc::terminate();

    std::cout << "Done.\n";
    return 0;
}
