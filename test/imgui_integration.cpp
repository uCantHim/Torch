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
    auto renderer = trc::init();
    auto scene = std::make_unique<trc::Scene>();
    trc::Camera camera;

    trc::imgui::initImgui(vkb::getDevice(), *renderer, vkb::getSwapchain());

    while (vkb::getSwapchain().isOpen())
    {
        vkb::pollEvents();

        trc::imgui::beginImguiFrame();
        ig::Begin("Window :D");
        ig::Text("Hello World!");
        ig::End();

        renderer->drawFrame(*scene, camera);
    }

    vkb::getDevice()->waitIdle();
    scene.reset();
    renderer.reset();
    trc::imgui::terminateImgui();
    trc::terminate();

    std::cout << "Done.\n";
    return 0;
}
