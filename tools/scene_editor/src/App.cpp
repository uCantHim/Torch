#include "App.h"

#include <chrono>
#include <memory>
#include <thread>

#include <trc/base/Logging.h>
#include <trc_util/Timer.h>

#include "asset/DefaultAssets.h"
#include "asset/HitboxAsset.h"
#include "gui/ContextMenu.h"
#include "input/KeyConfig.h"



App::App(const fs::path& projectRootDir)
    :
    initGlobalState([this]() -> bool {
        // Initialize global access
        if (_app != nullptr) {
            throw trc::Exception("[In App::App]: Only one App object may exist at any given time.");
        }
        _app = this;

        return true;
    }()),
    torchTerminator(new int(42), [](int* i) { delete i; trc::terminate(); }),
    torch(trc::initFull(
        trc::TorchStackCreateInfo{ .projectRootDir=projectRootDir },
        trc::InstanceCreateInfo{},
        trc::WindowCreateInfo{}
    )),
    imgui(trc::imgui::initImgui(torch->getWindow(), torch->getRenderConfig().getRenderGraph())),
    assetInventory(torch->getAssetManager(), torch->getAssetManager().getDataStorage()),
    scene(*this),
    mainMenu(*this)
{
    vec2 size = torch->getWindow().getWindowSize();
    torch->getRenderConfig().setViewport({ size.x * 0.25f, 0.0f }, size);
    scene.getCamera().setAspect(size.x / size.y);
    setSceneViewport({ size.x * 0.25f, 0.0f }, { size.x * 0.75f, size.y });

    // Initialize input
    trc::Keyboard::init();
    trc::Mouse::init();
    trc::on<trc::KeyPressEvent>([this](const trc::KeyPressEvent& e) {
        inputState.notify({ e.key, e.mods, trc::InputAction::press });
    });
    trc::on<trc::KeyRepeatEvent>([this](const trc::KeyRepeatEvent& e) {
        inputState.notify({ e.key, e.mods, trc::InputAction::repeat });
    });
    trc::on<trc::KeyReleaseEvent>([this](const trc::KeyReleaseEvent& e) {
        inputState.notify({ e.key, e.mods, trc::InputAction::release });
    });
    trc::on<trc::MouseClickEvent>([this](const trc::MouseClickEvent& e) {
        inputState.notify({ e.button, e.mods, trc::InputAction::press });
    });
    trc::on<trc::MouseReleaseEvent>([this](const trc::MouseReleaseEvent& e) {
        inputState.notify({ e.button, e.mods, trc::InputAction::release });
    });

    inputState.setKeyMap(makeKeyMap(*this,
        KeyConfig{
            .closeApp = trc::Key::escape,
            .openContext = trc::MouseButton::right,
            .selectHoveredObject = trc::MouseButton::left,
            .deleteHoveredObject = trc::Key::del,
            .cameraMove = { trc::MouseButton::middle, trc::KeyModFlagBits::shift },
            .cameraRotate = trc::MouseButton::middle,
            .translateObject = trc::Key::g,
            .scaleObject = trc::Key::s,
            .rotateObject = trc::Key::r,
        }
    ));

    // Initialize assets
    torch->getAssetManager().registerAssetType<HitboxAsset>(std::make_unique<HitboxRegistry>());
    assetInventory.detectAssetsFromStorage();

    auto preproc = [](AssetInventory& inventory,
                      const trc::AssetPath& geoPath,
                      const trc::AssetData<trc::Geometry>& data)
    {
        const auto hitbox = makeHitbox(data);
        HitboxData hitboxData{
            .sphere=hitbox.getSphere(),
            .capsule=hitbox.getCapsule(),
            .geometry=geoPath
        };
        const trc::AssetPath path(geoPath.string() + "_hitbox");

        inventory.import(path, hitboxData);
    };
    assetInventory.registerImportProcessor<trc::Geometry>(preproc);

    // Create default resources
    auto& ar = torch->getAssetManager();
    initDefaultAssets(ar);

    auto mg = ar.create(trc::makeMaterial({ .color=vec4(0, 0.6, 0, 1), .specularCoefficient=0.0f }));
    auto mr = ar.create(trc::makeMaterial({ .color=vec4(1, 0, 0, 1) }));
    auto mo = ar.create(trc::makeMaterial({ .color=vec4(1, 0.35f, 0, 1) }));

    auto planeData = trc::makePlaneGeo(20, 20, 1, 1);
    auto gi = ar.create(planeData);
    auto hb = makeHitbox(planeData);
    ar.create(HitboxData{
        .sphere=hb.getSphere(),
        .capsule=hb.getCapsule(),
        .geometry=gi
    });
    auto planeData1 = trc::makePlaneGeo(0.5f, 0.5f, 1, 1);
    auto gi1 = ar.create(planeData1);
    auto hb1 = makeHitbox(planeData1);
    ar.create(HitboxData{
        .sphere=hb1.getSphere(),
        .capsule=hb1.getCapsule(),
        .geometry=gi1
    });
    auto cubeGeo = ar.create(trc::makeCubeGeo());
    auto cubeHb = makeHitbox(trc::makeCubeGeo());
    ar.create(HitboxData{
        .sphere=cubeHb.getSphere(),
        .capsule=cubeHb.getCapsule(),
        .geometry=cubeGeo
    });

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

auto App::getAssets() -> AssetInventory&
{
    return assetInventory;
}

auto App::getScene() -> Scene&
{
    return scene;
}

void App::setSceneViewport(vec2 offset, vec2 size)
{
    torch->getRenderConfig().setViewport(offset, size);
    scene.getCamera().setAspect(size.x / size.y);
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

    torch->drawFrame(scene.getCamera(), scene.getDrawableScene());

    // Finalize
    static trc::Timer timer;
    std::chrono::milliseconds timeDiff(static_cast<i64>(30.0f - timer.duration()));
    std::this_thread::sleep_for(timeDiff);
    timer.reset();
}
