#include <trc/Torch.h>

int main()
{
    auto renderer = trc::init();
    auto scene = std::make_unique<trc::Scene>();
    trc::Camera camera;

    while (vkb::getSwapchain().isOpen())
    {
        renderer->drawFrame(*scene, camera);
        vkb::pollEvents();
    }

    renderer.reset();
    scene.reset();
    trc::terminate();

    return 0;
}
