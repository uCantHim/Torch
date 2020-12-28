#include <iostream>

#include <trc/Torch.h>

int main()
{
    auto renderer = trc::init({ .enableRayTracing=true });
    auto scene = std::make_unique<trc::Scene>();
    trc::Camera camera;

    while (vkb::getSwapchain().isOpen())
    {
        vkb::pollEvents();
        renderer->drawFrame(*scene, camera);
    }

    trc::terminate();
    return 0;
}
