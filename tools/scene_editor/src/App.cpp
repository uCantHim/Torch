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
    assetManager(&torch->getAssetManager()),
    scene(*this),
    mainMenu(*this)
{
    // Initialize global access
    if (_app != nullptr) {
        throw trc::Exception("[In App::App]: Only one App object may exist at any give time.");
    }
    _app = this;

    // Initialize input
    vkb::Keyboard::init();
    vkb::Mouse::init();
    vkb::on<vkb::KeyPressEvent>([this](const vkb::KeyPressEvent& e) {
        inputState.notify({ e.key, e.mods, vkb::InputAction::press });
    });
    vkb::on<vkb::KeyRepeatEvent>([this](const vkb::KeyRepeatEvent& e) {
        inputState.notify({ e.key, e.mods, vkb::InputAction::repeat });
    });
    vkb::on<vkb::KeyReleaseEvent>([this](const vkb::KeyReleaseEvent& e) {
        inputState.notify({ e.key, e.mods, vkb::InputAction::release });
    });
    vkb::on<vkb::MouseClickEvent>([this](const vkb::MouseClickEvent& e) {
        inputState.notify({ e.button, e.mods, vkb::InputAction::press });
    });
    vkb::on<vkb::MouseReleaseEvent>([this](const vkb::MouseReleaseEvent& e) {
        inputState.notify({ e.button, e.mods, vkb::InputAction::release });
    });

    inputState.setKeyMap(makeKeyMap(*this,
        KeyConfig{
            .closeApp = vkb::Key::escape,
            .openContext = vkb::MouseButton::right,
            .selectHoveredObject = vkb::MouseButton::left,
            .translateObject = vkb::Key::g,
            .scaleObject = vkb::Key::s,
            .rotateObject = vkb::Key::r,
        }
    ));

    // Create resources
    initDefaultAssets(*assetManager);

    auto& ar = getAssets();
    auto mg = ar.create(trc::MaterialData{ .color=vec4(0, 0.6, 0, 1), .specularKoefficient=vec4(0.0f) });
    auto mr = ar.create(trc::MaterialData{ .color=vec4(1, 0, 0, 1) });
    auto mo = ar.create(trc::MaterialData{ .color=vec4(1, 0.35f, 0, 1) });

    auto planeData = trc::makePlaneGeo(20, 20, 1, 1);
    auto gi = ar.create(planeData);
    addHitbox(gi, makeHitbox(planeData));
    auto planeData1 = trc::makePlaneGeo(0.5f, 0.5f, 1, 1);
    auto gi1 = ar.create(planeData1);
    addHitbox(gi1, makeHitbox(planeData1));
    auto cubeGeo = ar.create(trc::makeCubeGeo());
    addHitbox(cubeGeo, makeHitbox(trc::makeCubeGeo()));

    scene.createDefaultObject({ gi, mg });

    auto smallPlane = scene.createDefaultObject({ gi1, mr });
    scene.get<ObjectBaseNode>(smallPlane).rotateX(glm::radians(90.0f)).translateY(1.5f);

    auto cube = scene.createDefaultObject({ cubeGeo, mo });
    scene.get<ObjectBaseNode>(cube).translateY(0.5f);

    frameTimer.reset();
}

App::~App()
{
    torch->getWindow().getRenderer().waitForAllFrames();
    _app = nullptr;
}

auto App::get() -> App&
{
    assert(_app != nullptr);
    return *_app;
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

auto App::getAssets() -> trc::AssetManager&
{
    assert(assetManager != nullptr);
    return *assetManager;
}

auto App::getScene() -> Scene&
{
    return scene;
}

void App::addHitbox(trc::GeometryID geo, Hitbox hitbox)
{
    hitboxes.try_emplace(geo.getDeviceID(), hitbox);
}

auto App::getHitbox(trc::GeometryID geo) const -> const Hitbox&
{
    return hitboxes.at(geo.getDeviceID());
}

void App::tick()
{
    assert(torch != nullptr);

    const float frameTime = frameTimer.reset();

    // Update
    trc::pollEvents();
    inputState.update(frameTime);
    scene.update(frameTime);

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
