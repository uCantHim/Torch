#include "App.h"

#include <chrono>
#include <memory>
#include <thread>

#include <trc/ImageClear.h>
#include <trc/ImguiIntegration.h>
#include <trc/base/Logging.h>
#include <trc_util/Timer.h>

#include "asset/DefaultAssets.h"
#include "asset/HitboxAsset.h"
#include "gui/AssetEditor.h"
#include "gui/ObjectBrowser.h"
#include "gui/SceneEditorFileExplorer.h"
#include "input/InputProcessor.h"
#include "input/KeyConfig.h"
#include "viewport/SceneViewport.h"



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
                [](auto&&) {
                    return trc::buildRenderTargetImageClearPlugin(
                        vk::ClearColorValue{ kClearColor.r, kClearColor.g, kClearColor.b, 1.0f }
                    );
                },
            },
            .assetStorageDir=projectRootDir/"assets",
        },
        trc::InstanceCreateInfo{},
        trc::WindowCreateInfo{
            .swapchainCreateInfo{
                .imageUsage = vk::ImageUsageFlagBits::eTransferDst,
            }
        }
    )),
    renderer(torch->getDevice(), torch->getWindow()),

    // Set up asset management.
    assetInventory(torch->getAssetManager(), torch->getAssetManager().getDataStorage()),

    // Create the main scene.
    camera(std::make_shared<trc::Camera>()),
    drawableScene(std::make_shared<trc::Scene>()),
    scene(std::make_shared<Scene>(*this, camera, drawableScene)),

    windowManager(std::make_shared<InputProcessor>()),

    // Create the always-present main scene viewport
    sceneViewport(std::make_unique<SceneViewport>(
        torch->getRenderPipeline(),
        camera,
        drawableScene,
        ViewportArea{
            { torch->getWindow().getSize().x * 0.25f, 0.0f },
            { torch->getWindow().getSize().x * 0.75f, torch->getWindow().getSize().y }
        }
    )),

    // Create the viewport manager
    mainWindowViewportManager(std::make_shared<ViewportTree>(
        ViewportArea{ { 0, 0 }, torch->getWindow().getSize() },
        sceneViewport
    ))
{
    // Initialize the window manager
    torch->getWindow().setInputProcessor(windowManager);
    windowManager->setRootViewport(torch->getWindow(), mainWindowViewportManager);

    // Initialize main viewport
    auto fileExplorer = std::make_shared<gui::SceneEditorFileExplorer>();
    auto assetBrowser = std::make_shared<gui::AssetEditor>();
    auto objectBrowser = std::make_shared<gui::ObjectBrowser>(scene);
    mainWindowViewportManager->createSplit(
        sceneViewport.get(),
        SplitInfo{
            .horizontal=false,
            .location=SplitLocation::makeRelative(0.25f),
        },
        assetBrowser,
        ViewportLocation::eFirst
    );
    mainWindowViewportManager->createSplit(
        assetBrowser.get(),
        SplitInfo{
            .horizontal=true,
            .location=SplitLocation::makePixel(300u),
        },
        objectBrowser,
        ViewportLocation::eSecond
    );

    fileExplorer->setWindowType(ImguiWindowType::eFloating);
    mainWindowViewportManager->createFloating(
        std::move(fileExplorer),
        ViewportArea{ { sceneViewport->getSize().pos.x + 30, 30 }, { 300, 300 } }
    );

    torch->getWindow().addCallbackOnResize([this](trc::Swapchain& swapchain) {
        mainWindowViewportManager->resize({ { 0, 0 }, swapchain.getWindowSize() });
    });

    // Initialize input
    KeyConfig keyConfig{
        .closeApp = trc::Key::escape,
        .openContext = trc::MouseButton::right,
        .selectHoveredObject = trc::MouseButton::left,
        .deleteHoveredObject = trc::Key::del,
        .cameraMove = { trc::MouseButton::middle, trc::KeyModFlagBits::shift },
        .cameraRotate = trc::MouseButton::middle,
        .translateObject = trc::Key::g,
        .scaleObject = trc::Key::s,
        .rotateObject = trc::Key::r,
    };
    setupRootInputFrame(mainWindowViewportManager->getRootInputHandler(), keyConfig, *this);
    setupMainSceneInputFrame(sceneViewport->getInputHandler(), keyConfig, *this);
    sceneViewport->getInputHandler().on(trc::Key::a, []{ std::cout << "A!\n"; });

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
    torch->getWindow().setInputProcessor(std::make_unique<trc::NullInputProcessor>());

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

auto App::getViewportManager() -> ViewportTree&
{
    return *mainWindowViewportManager;
}

auto App::getSceneViewport() -> ViewportArea
{
    return sceneViewport->getSize();
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

    auto frame = torch->getRenderPipeline().makeFrame();
    mainWindowViewportManager->draw(*frame);
    renderer.renderFrameAndPresent(std::move(frame), torch->getWindow());

    // Finalize
    static trc::Timer timer;
    std::chrono::milliseconds timeDiff(static_cast<i64>(30.0f - timer.duration()));
    std::this_thread::sleep_for(timeDiff);
    timer.reset();
}
