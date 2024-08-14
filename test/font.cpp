#include <trc/Torch.h>
#include <trc/drawable/DrawableScene.h>
#include <trc/text/Font.h>
#include <trc/text/Text.h>

int main()
{
    {
        auto torch = trc::initFull();
        auto& instance = torch->getInstance();
        auto& window = torch->getWindow();
        auto& assets = torch->getAssetManager();

        auto scene = std::make_shared<trc::DrawableScene>();
        auto camera = std::make_shared<trc::Camera>();
        camera->lookAt({ -1.0f, 1.0f, 2.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f });
        camera->setDepthBounds(0.1f, 100.0f);

        // Font stuff
        auto font = assets.create(trc::loadFont(TRC_TEST_FONT_DIR"/gil.ttf", 60));

        trc::Text text(instance, font.getDeviceDataHandle());
        text.print("^Hello{ | }\n ~World_!$ ✓");
        text.attachToScene(scene->getRasterModule());
        // ---

        // Main loop
        auto vp = torch->makeFullscreenViewport(camera, scene);
        while (window.isOpen())
        {
            trc::pollEvents();
            torch->drawFrame(vp);
        }

        torch->waitForAllFrames();
    }

    trc::terminate();

    return 0;
}
