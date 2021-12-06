#include <trc/Torch.h>
#include <trc/text/Font.h>
#include <trc/text/Text.h>

int main()
{
    {
        auto torch = trc::initFull();
        auto& instance = *torch.instance;
        auto& fonts = torch.assetRegistry->getFonts();

        // Font stuff

        trc::Font font = fonts.makeFont(TRC_TEST_FONT_DIR"/gil.ttf", 60);

        trc::Text text(instance, font);
        text.print("^Hello{ | }\n ~World_!$");

        // ---

        trc::Scene scene;
        trc::Camera camera;
        camera.lookAt({ -1.0f, 1.0f, 2.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f });
        camera.setDepthBounds(0.1f, 100.0f);

        text.attachToScene(scene);

        trc::DrawConfig draw{
            .scene        = &scene,
            .camera       = &camera,
            .renderConfig = &*torch.renderConfig
        };

        // Main loop
        while (torch.window->getSwapchain().isOpen())
        {
            vkb::pollEvents();
            torch.window->drawFrame(draw);
        }
    }

    trc::terminate();

    return 0;
}
