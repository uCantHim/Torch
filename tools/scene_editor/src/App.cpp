#include "App.h"

#include <thread>
#include <chrono>

#include <trc_util/Timer.h>

#include "GlobalState.h"



void App::start(int, char*[])
{
    init();
    while (!doEnd)
    {
        tick();
    }
    terminate();
}

void App::end()
{
    doEnd = true;
}

auto App::getTorch() -> trc::TorchStack&
{
    return *torch;
}

auto App::getAssets() -> trc::AssetRegistry&
{
    return torch->getAssetRegistry();
}

auto App::getScene() -> Scene&
{
    return *scene;
}

void App::init()
{
    torch = trc::initFull();
    imgui = trc::imgui::initImgui(torch->getWindow(), torch->getRenderConfig().getLayout());

    vkb::Keyboard::init();
    vkb::Mouse::init();
    vkb::on<vkb::KeyPressEvent>([](const vkb::KeyPressEvent& e) {
        if (e.key == vkb::Key::escape) { App::end(); }
    });

    scene = std::make_unique<Scene>();

    // Create GUI
    vkb::on<vkb::MouseClickEvent>([](const auto& e) {
        if (e.button == vkb::MouseButton::right) {
            gui::util::ContextMenu::show("object", []() { ig::Text("Hello context!"); });
        }
    });

    // Create resources
    auto& ar = torch->getAssetRegistry().named;
    auto mg = ar.add("green", trc::Material{ .color = vec4(0, 0.6, 0, 1), .kSpecular = vec4(0.0f) });
    auto mr = ar.add("red", trc::Material{ .color = vec4(1, 0, 0, 1) });

    auto gi = ar.add("ground_plane", trc::makePlaneGeo(20, 20, 1, 1));
    auto gi1 = ar.add("plane", trc::makePlaneGeo(0.5f, 0.5f, 1, 1));
    auto cubeGeo = ar.add("cube", trc::makeCubeGeo());

    scene->addObject(std::make_unique<trc::Drawable>(gi, mg));

    auto& smallPlane = scene->getObject(scene->addObject(std::make_unique<trc::Drawable>(gi1, mr)));
    smallPlane.getSceneNode().rotateX(glm::radians(90.0f)).translateY(1.5f);

    scene->addObject(std::make_unique<trc::Drawable>(cubeGeo, mr));
}

void App::tick()
{
    assert(torch != nullptr);
    assert(scene != nullptr);

    // Update
    trc::pollEvents();
    scene->update();

    // Render
    trc::imgui::beginImguiFrame();
    mainMenu.drawImGui();
    gui::util::ContextMenu::drawImGui();

    torch->drawFrame(torch->makeDrawConfig(scene->getDrawableScene(), scene->getCamera()));

    // Finalize
    static trc::Timer timer;
    std::chrono::milliseconds timeDiff(static_cast<i64>(30.0f - timer.duration()));
    std::this_thread::sleep_for(timeDiff);
    timer.reset();
}

void App::terminate()
{
    imgui.reset();
    scene.reset();
    torch.reset();

    trc::terminate();
}
