#include "App.h"

#include <chrono>
#include <memory>
#include <thread>

#include <imgui.h>
#include <trc/ImageClear.h>
#include <trc/ImguiIntegration.h>
#include <trc/base/Logging.h>
#include <trc/util/FilesystemDataStorage.h>
#include <trc_util/Timer.h>

#include "asset/DefaultAssets.h"
#include "asset/HitboxAsset.h"
#include "graphics/SceneRenderPipeline.h"
#include "gui/AssetEditor.h"
#include "gui/ObjectBrowser.h"
#include "gui/SceneEditorFileExplorer.h"
#include "input/KeyConfig.h"
#include "viewport/SceneViewport.h"

namespace ig = ImGui;



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

    // Set up graphics
    graphics(),
    mainTorchWindow(std::make_unique<trc::Window>(
        graphics.getInstance(),
        trc::WindowCreateInfo{
            .swapchainCreateInfo{
                // Required to be able to clear swapchain images via vkCmdClearColorImage
                .imageUsage = vk::ImageUsageFlagBits::eTransferDst,
            }
        }
    )),

    // Set up asset management.
    assetDataStorage(std::make_shared<trc::FilesystemDataStorage>(projectRootDir/"assets")),
    assetManager(assetDataStorage),
    assetInventory(assetManager, assetManager.getDataStorage()),

    // Create the main scene.
    camera(std::make_shared<trc::Camera>()),
    drawableScene(std::make_shared<trc::Scene>()),
    scene(std::make_shared<Scene>(*this, camera, drawableScene)),

    // Create the always-present main scene viewport
    sceneRenderPipeline(makeSceneRenderPipeline(*mainTorchWindow, assetManager.getDeviceRegistry())),
    sceneViewport(std::make_unique<SceneViewport>(
        *sceneRenderPipeline,
        camera,
        drawableScene,
        ViewportArea{ { 0, 0 }, { 1, 1 } }
    )),

    // Set up viewport management for the main window
    mainWindow(std::make_shared<Window>(
        graphics,
        mainTorchWindow,
        sceneViewport
    ))
{
    mainTorchWindow->addCallbackAfterSwapchainRecreate(
        [renderer=mainWindow->renderer.get(), pipeline=sceneRenderPipeline.get()](trc::Swapchain& sc)
        {
            renderer->waitForAllFrames();
            pipeline->changeRenderTarget(makeRenderTarget(sc));
        }
    );

    // Initialize main viewport
    auto fileExplorer = std::make_shared<gui::SceneEditorFileExplorer>();
    auto assetBrowser = std::make_shared<gui::AssetEditor>();
    auto objectBrowser = std::make_shared<gui::ObjectBrowser>(scene);
    mainWindow->viewportTree->createSplit(
        sceneViewport.get(),
        SplitInfo{
            .horizontal=false,
            .location=SplitLocation::makeRelative(0.25f),
        },
        assetBrowser,
        ViewportLocation::eFirst
    );
    mainWindow->viewportTree->createSplit(
        assetBrowser.get(),
        SplitInfo{
            .horizontal=true,
            .location=SplitLocation::makePixel(300u),
        },
        objectBrowser,
        ViewportLocation::eSecond
    );

    fileExplorer->setWindowType(ImguiWindowType::eFloating);
    mainWindow->viewportTree->createFloating(
        std::move(fileExplorer),
        ViewportArea{ { sceneViewport->getSize().pos.x + 30, 30 }, { 300, 300 } }
    );

    mainWindow->torchWindow->addCallbackOnResize([this](trc::Swapchain& swapchain) {
        mainWindow->viewportTree->resize({ { 0, 0 }, swapchain.getWindowSize() });
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
    setupRootInputFrame(mainWindow->rootViewport->getInputHandler(), keyConfig);
    setupMainSceneInputFrame(sceneViewport->getInputHandler(), keyConfig, scene);

    // Disable imgui setting the mouse cursor image. We do this ourselves (see
    // `tick`) because the viewport manager also needs to change the cursor.
    ig::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;

    // Initialize assets
    assetManager.registerAssetType<HitboxAsset>(std::make_unique<HitboxRegistry>());
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
    auto& ar = assetManager;
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
    graphics.getDevice()->waitIdle();
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

auto App::getMainWindow() -> trc::Window&
{
    return *mainTorchWindow;
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
    return *mainWindow->viewportTree;
}

auto App::getSceneViewport() -> ViewportArea
{
    return sceneViewport->getSize();
}

void App::tick()
{
    const float frameTime = frameTimer.reset();

    // Update
    trc::pollEvents();
    scene->update(frameTime);

    // Render
    trc::imgui::beginImguiFrame();

    auto frame = sceneRenderPipeline->makeFrame();
    mainWindow->drawContent(*frame);
    mainWindow->submitFrame(std::move(frame));

    // Finalize
    static trc::Timer timer;
    std::chrono::milliseconds timeDiff(static_cast<i64>(30.0f - timer.duration()));
    std::this_thread::sleep_for(timeDiff);
    timer.reset();

    if (mainWindow->torchWindow->shouldClose()) {
        end();
    }
}

void App::setupRootInputFrame(InputFrame& f, const KeyConfig& conf)
{
    f.on(conf.closeApp, [&]{ this->end(); });
}
