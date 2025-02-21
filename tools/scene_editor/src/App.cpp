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
#include "input/InputProcessor.h"



ViewportManager::ViewportManager(App& app, u_ptr<SceneViewport> sceneVp)
    :
    mainMenuViewport(app),
    sceneViewport(std::move(sceneVp))
{
}

void ViewportManager::resize(const ViewportArea& newArea)
{
    currentArea = newArea;

    const auto [_, size] = newArea;
    sceneViewport->resize({
        { size.x * horizontalSceneVpPos,          0.0f },
        { size.x * (1.0f - horizontalSceneVpPos), size.y },
    });
}

auto ViewportManager::getSize() -> ViewportArea
{
    return currentArea;
}

void ViewportManager::setSceneViewportPos(float horizontalPos)
{
    horizontalSceneVpPos = horizontalPos;
}

void ViewportManager::notify(const UserInput& input)
{
    sceneViewport->notify(input);
}

void ViewportManager::notify(const Scroll& scroll)
{
    sceneViewport->notify(scroll);
}

void ViewportManager::notify(const CursorMovement& cursorMove)
{
    const auto [off, size] = sceneViewport->getSize();
    sceneViewport->notify(CursorMovement{
        .position = cursorMove.position - vec2{off},
        .offset = cursorMove.offset,
        .areaSize = size,
    });
}



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

    // Create torch.
    torchTerminator(new int(42), [](int* i) { delete i; trc::terminate(); }),
    torch(trc::initFull(
        trc::TorchStackCreateInfo{
            .plugins{
                trc::imgui::buildImguiRenderPlugin,
            },
            .assetStorageDir=projectRootDir/"assets",
        },
        trc::InstanceCreateInfo{},
        trc::WindowCreateInfo{}
    )),

    // Set up asset management.
    assetInventory(torch->getAssetManager(), torch->getAssetManager().getDataStorage()),

    // Create the main scene.
    camera(std::make_shared<trc::Camera>()),
    drawableScene(std::make_shared<trc::Scene>()),
    scene(nullptr),

    // Create the viewport manager
    sceneVp(torch->makeViewport(
        trc::RenderArea{
            { torch->getWindow().getSize().x * 0.25f, 0.0f },
            { torch->getWindow().getSize().x * 0.75f, torch->getWindow().getSize().y }
        },
        camera,
        drawableScene
    )),
    viewportManager(std::make_shared<ViewportManager>(
        *this,
        std::make_unique<SceneViewport>(sceneVp)
        //std::make_unique<SceneViewport>(
        //    torch->getWindow(),
        //    assetInventory,
        //    ViewportArea{
        //        { torch->getWindow().getSize().x * 0.25f, 0.0f },
        //        { torch->getWindow().getSize().x * 0.75f, torch->getWindow().getSize().y }
        //    },
        //    drawableScene,
        //    camera
        //)
    )),
    viewportTree(std::make_shared<ViewportTree>(viewportManager))
{
    // Create a scene
    scene = std::make_shared<Scene>(*this, camera, drawableScene);

    // Initialize viewport
    viewportManager->setSceneViewportPos(0.25f);
    torch->getWindow().addCallbackOnResize([this](trc::Swapchain& swapchain) {
        viewportTree->resize({ { 0, 0 }, swapchain.getWindowSize() });
    });

    // Initialize input
    torch->getWindow().setInputProcessor(std::make_shared<InputProcessor>(viewportTree));
    setupRootInputFrame(
        viewportManager->sceneViewport->getInputHandler(),
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
        },
        *this
    );

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
            .box=hitbox.getBox(),
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
        .box=hb.getBox(),
        .geometry=gi
    });
    auto planeData1 = trc::makePlaneGeo(0.5f, 0.5f, 1, 1);
    auto gi1 = ar.create(planeData1);
    auto hb1 = makeHitbox(planeData1);
    ar.create(HitboxData{
        .sphere=hb1.getSphere(),
        .capsule=hb1.getCapsule(),
        .box=hb1.getBox(),
        .geometry=gi1
    });
    auto cubeGeo = ar.create(trc::makeCubeGeo());
    auto cubeHb = makeHitbox(trc::makeCubeGeo());
    ar.create(HitboxData{
        .sphere=cubeHb.getSphere(),
        .capsule=cubeHb.getCapsule(),
        .box=cubeHb.getBox(),
        .geometry=cubeGeo
    });

    scene->createDefaultObject({ gi, mg });

    auto smallCube = scene->createDefaultObject({ cubeGeo, mr });
    scene->get<ObjectBaseNode>(smallCube).rotateX(glm::radians(90.0f)).translateY(1.5f).scale(0.2f);

    auto cube = scene->createDefaultObject({ cubeGeo, mo });
    scene->get<ObjectBaseNode>(cube).translateY(0.5f);

    frameTimer.reset();
}

App::~App()
{
    torch->waitForAllFrames();
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
    return *scene;
}

auto App::getSceneViewport() -> ViewportArea
{
    return viewportManager->sceneViewport->getSize();
}

void App::tick()
{
    assert(torch != nullptr);

    const float frameTime = frameTimer.reset();

    // Update
    trc::pollEvents();
    scene->update(frameTime);

    // Render
    trc::imgui::beginImguiFrame();
    viewportManager->mainMenuViewport.drawImGui();
    gui::ContextMenu::drawImGui();

    torch->drawFrame(sceneVp);

    // Finalize
    static trc::Timer timer;
    std::chrono::milliseconds timeDiff(static_cast<i64>(30.0f - timer.duration()));
    std::this_thread::sleep_for(timeDiff);
    timer.reset();
}
