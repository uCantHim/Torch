#include <trc/Torch.h>
#include <trc/text/Font.h>
#include <trc/text/Text.h>

int main()
{
    {
        auto torch = trc::initFull();
        auto& instance = torch->getInstance();
        auto& window = torch->getWindow();
        auto& fonts = torch->getAssetManager().getDeviceRegistry().getFonts();

        trc::Scene scene;
        trc::Camera camera;
        camera.lookAt({ -1.0f, 1.0f, 2.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f });
        camera.setDepthBounds(0.1f, 100.0f);


        // Font stuff

        trc::Font font(fonts, TRC_TEST_FONT_DIR"/gil.ttf", 60);

        trc::Text text(instance, font);
        text.print("^Hello{ | }\n ~World_!$ ✓");
        text.attachToScene(scene);

        // ---


        // Main loop
        while (window.isOpen())
        {
            vkb::pollEvents();
            window.drawFrame(torch->makeDrawConfig(scene, camera));
        }

        window.getRenderer().waitForAllFrames();
    }

    trc::terminate();

    return 0;
}
