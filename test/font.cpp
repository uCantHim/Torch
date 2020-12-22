#include <trc/Torch.h>
#include <trc/Font.h>
#include <trc/Text.h>

int main()
{
    auto renderer = trc::init();

    // Font stuff

    trc::Font font{ "fonts/gil.ttf", 30 };
    for (trc::CharCode c = 0; c < 300; c++) {
        font.getGlyph(c);
    }

    trc::Text text(font);
    text.print("^Hello{ | }\n ~World_!$");

    // ---

    auto scene = std::make_unique<trc::Scene>();
    trc::Camera camera;

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
