#include <iostream>

#include <argparse/argparse.hpp>
#include <trc/Torch.h>
#include <trc/assets/Material.h>
#include <trc/base/event/Event.h>
#include <trc_util/Timer.h>
using namespace trc::basic_types;

#include "load_utils.h"

constexpr auto kInvalidUsageExitcode{ 64 };
constexpr const char* description = "Display a material in a preview window.";

void display(const trc::MaterialData& mat);

int main(int argc, const char* argv[])
{
    argparse::ArgumentParser program;
    program.add_description(description);

    program.add_argument("file")
        .help("The Torch material file to inspect.");

    // Parse command-line args
    try {
        program.parse_args(argc, argv);
    }
    catch (const std::runtime_error& err) {
        std::cout << program;
        exit(kInvalidUsageExitcode);
    }

    // Try to open file
    auto mat = tryLoad<trc::Material>(fs::path{ program.get("file") });
    if (!mat) {
        std::cout << "Error: " << mat.error() << ". Exiting.\n";
        exit(1);
    }

    // Display the material
    display(*mat);

    return 0;
}

void display(const trc::MaterialData& mat)
{
    static constexpr vec2 windowSize{ 800, 600 };
    static constexpr vec3 sunLightColor{ 0.957f, 0.918f, 0.608f };

    auto torch = trc::initFull(
        {.assetStorageDir="."},
        trc::InstanceCreateInfo{ .enableRayTracing=false },
        trc::WindowCreateInfo{
            .size=windowSize,
            .title="Material preview",
        }
    );

    auto camera = std::make_shared<trc::Camera>();
    auto scene = std::make_shared<trc::Scene>();
    auto vp = torch->makeFullscreenViewport(camera, scene);

    // Set up the camera
    camera->makePerspective(torch->getWindow().getAspectRatio(), 45.0f, 0.1f, 50.0f);
    camera->lookAt({ 0, 0, 4 }, { 0, 0, 0 }, { 0, 1, 0 });

    // Set up the scene
    auto drawable = scene->makeDrawable(trc::DrawableCreateInfo{
        .geo=torch->getAssetManager().create(trc::makeSphereGeo()),
        .mat=torch->getAssetManager().create(mat),
        .rasterized=true,
        .rayTraced=false,
    });

    auto light = scene->getLights().makeSunLight(sunLightColor, vec3(1, -1, -1), 0.6f);

    bool needsRedraw{ true };
    trc::on<trc::WindowResizeEvent>([&](auto&& e) {
        camera->setAspect(e.swapchain->getAspectRatio());
        needsRedraw = true;
    });
    while (!torch->getWindow().isPressed(trc::Key::escape) && torch->getWindow().isOpen())
    {
        trc::pollEvents();
        if (needsRedraw)
        {
            torch->drawFrame(vp);
            needsRedraw = false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    trc::terminate();
}
