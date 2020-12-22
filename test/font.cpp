#include <trc/Torch.h>
#include <trc/Font.h>
#include <trc/Text.h>

int main()
{
    auto renderer = trc::init();

    // Font stuff

    trc::Font font{ "fonts/gil.ttf", 60 };

    trc::Text text(font);
    text.print("^Hello{ | }\n ~World_!$");

    // ---

    auto scene = std::make_unique<trc::Scene>();
    trc::Camera camera;
    camera.lookAt({ -1.0f, 1.0f, 2.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f });
    camera.setDepthBounds(0.1f, 100.0f);

    text.attachToScene(*scene);

    // Main loop
    while (vkb::getSwapchain().isOpen())
    {
        vkb::pollEvents();
        renderer->drawFrame(*scene, camera);
    }

    // Destroy the Torch resources
    renderer.reset();
    scene.reset();
    trc::terminate();

    return 0;
}
