#include <iostream>

#include <trc/ImguiIntegration.h>
#include <trc/Torch.h>

namespace ig = ImGui;

int main()
{
    {
        auto torch = trc::initFull({
            .plugins{
                trc::imgui::buildImguiRenderPlugin,
            }
        });

        auto scene = std::make_shared<trc::Scene>();
        auto camera = std::make_shared<trc::Camera>();
        auto vp = torch->makeFullscreenViewport(camera, scene);

        while (torch->getWindow().isOpen())
        {
            trc::pollEvents();

            trc::imgui::beginImguiFrame();
            ig::Begin("Window :D");
            ig::Text("Hello World!");
            ig::End();

            torch->drawFrame(vp);
        }

        torch->waitForAllFrames();
    }

    trc::terminate();

    std::cout << "Done.\n";
    return 0;
}
