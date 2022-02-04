#include "App.h"

#include <thread>
#include <chrono>

#include <trc_util/Timer.h>

#include "gui/ContextMenu.h"
#include "DefaultAssets.h"
#include "input/KeyConfig.h"



App::App(int, char*[])
    :
    trcTerminator(new int(42), [](int* i) { delete i; trc::terminate(); }),
    torch(trc::initFull()),
    imgui(trc::imgui::initImgui(torch->getWindow(), torch->getRenderConfig().getLayout())),
    assetManager(torch->getAssetRegistry()),
    scene(*this),
    mainMenu(*this),
    inputState(*this)
{
    vkb::Keyboard::init();
    vkb::Mouse::init();
    vkb::on<vkb::KeyPressEvent>([this](const vkb::KeyPressEvent& e) {
        inputState.notify({ e.key, e.mods });
    });
    vkb::on<vkb::MouseClickEvent>([this](const vkb::MouseClickEvent& e) {
        inputState.notify({ e.button, e.mods });
    });

    inputState.setKeyMap(makeKeyMap(*this,
        KeyConfig{
            .closeApp = vkb::Key::escape,
            .openContext = vkb::MouseButton::right
        }
    ));

    // Create resources
    initDefaultAssets(assetManager);

    auto& ar = getAssets();
    auto mg = ar.add(trc::Material{ .color = vec4(0, 0.6, 0, 1), .kSpecular = vec4(0.0f) });
    auto mr = ar.add(trc::Material{ .color = vec4(1, 0, 0, 1) });
    auto mo = ar.add(trc::Material{ .color = vec4(1, 0.35f, 0, 1) });

    auto gi = ar.add(trc::makePlaneGeo(20, 20, 1, 1));
    auto gi1 = ar.add(trc::makePlaneGeo(0.5f, 0.5f, 1, 1));
    auto cubeGeo = ar.add(trc::makeCubeGeo());

    scene.createDefaultObject(trc::Drawable(gi, mg));

    auto smallPlane = scene.createDefaultObject(trc::Drawable(gi1, mr));
    scene.get<ObjectBaseNode>(smallPlane).rotateX(glm::radians(90.0f)).translateY(1.5f);

    auto cube = scene.createDefaultObject(trc::Drawable(cubeGeo, mo));
    scene.get<ObjectBaseNode>(cube).translateY(0.5f);

    frameTimer.reset();
}

App::~App()
{
    torch->getWindow().getRenderer().waitForAllFrames();
}

void App::run()
{
    while (!doEnd)
    {
        tick();
    }
}

void App::end()
{
    doEnd = true;
}

auto App::getTorch() -> trc::TorchStack&
{
    return *torch;
}

auto App::getAssets() -> AssetManager&
{
    return assetManager;
}

auto App::getScene() -> Scene&
{
    return scene;
}

void App::tick()
{
    assert(torch != nullptr);

    const float frameTime = frameTimer.reset();

    // Update
    trc::pollEvents();
    inputState.update(frameTime);
    scene.update();

    // Render
    trc::imgui::beginImguiFrame();
    mainMenu.drawImGui();
    gui::ContextMenu::drawImGui();

    torch->drawFrame(torch->makeDrawConfig(scene.getDrawableScene(), scene.getCamera()));

    // Finalize
    static trc::Timer timer;
    std::chrono::milliseconds timeDiff(static_cast<i64>(30.0f - timer.duration()));
    std::this_thread::sleep_for(timeDiff);
    timer.reset();
}
