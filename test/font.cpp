#include <trc/Torch.h>
#include <trc/text/Font.h>
#include <trc/text/Text.h>

int main()
{
    {
        auto torch = trc::initFull();
        auto& instance = torch->getInstance();
        auto& window = torch->getWindow();
        auto& assets = torch->getAssetManager();

        trc::RasterSceneModule scene;
        trc::Camera camera;
        camera.lookAt({ -1.0f, 1.0f, 2.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f });
        camera.setDepthBounds(0.1f, 100.0f);


        // Font stuff

        auto font = assets.create(trc::loadFont(TRC_TEST_FONT_DIR"/gil.ttf", 60));

        trc::Text text(instance, font.getDeviceDataHandle());
        text.print("^Hello{ | }\n ~World_!$ ✓");
        text.attachToScene(scene);

        // ---


        // Main loop
        while (window.isOpen())
        {
            trc::pollEvents();
            torch->drawFrame(camera, scene);
        }

        window.getRenderer().waitForAllFrames();
    }

    trc::terminate();

    return 0;
}
