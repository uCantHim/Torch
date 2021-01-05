#include <iostream>

#include <trc/Torch.h>
#include <trc/experimental/ImguiIntegration.h>

int main()
{
    auto renderer = trc::init();
    auto scene = std::make_unique<trc::Scene>();
    trc::Camera camera;

    trc::experimental::initImgui(vkb::getDevice(), *renderer, vkb::getSwapchain());

    while (vkb::getSwapchain().isOpen())
    {
        vkb::pollEvents();
        renderer->drawFrame(*scene, camera);
    }

    vkb::getDevice()->waitIdle();
    scene.reset();
    renderer.reset();
    trc::terminate();

    std::cout << "Done.\n";
    return 0;
}
