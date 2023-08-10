#include <iostream>

#include <argparse/argparse.hpp>
#include <trc/Torch.h>
#include <trc/assets/Texture.h>
using namespace trc::basic_types;

#include "load_utils.h"

constexpr auto kInvalidUsageExitcode{ 64 };
constexpr const char* description = "Print some information about a texture and"
                                    " display it in a preview window.";

void printInfo(const trc::TextureData& tex)
{
    std::cout << "Size: " << tex.size.x << "x" << tex.size.y << "\n";
}

void display(const trc::TextureData& tex, float maxDuration);

int main(int argc, const char* argv[])
{
    argparse::ArgumentParser program;
    program.add_description(description);

    program.add_argument("file")
        .help("The Torch texture file to inspect.");

    program.add_argument("--no-text")
        .default_value(false)
        .implicit_value(true)
        .help("Don't print information text to stdout.");
    program.add_argument("--no-display")
        .default_value(false)
        .implicit_value(true)
        .help("Don't display the texture graphically.");
    program.add_argument("--display-duration", "--max-duration")
        .scan<'f', float>()
        .help("Automatically close the display after this many seconds.");

    // Parse command-line args
    try {
        program.parse_args(argc, argv);
    }
    catch (const std::runtime_error& err) {
        std::cout << program;
        exit(kInvalidUsageExitcode);
    }

    // Try to open file
    auto tex = tryLoad<trc::Texture>(fs::path{ program.get("file") });
    if (!tex) {
        std::cout << "Error: " << tex.error() << ". Exiting.\n";
        exit(1);
    }

    // Process geometry with the applicable output methods
    if (!program.get<bool>("--no-text")) {
        printInfo(*tex);
    }
    if (!program.get<bool>("--no-display"))
    {
        const float maxDuration = program.present<float>("--display-duration")
                                  .value_or(std::numeric_limits<float>::max());
        display(*tex, maxDuration);
    }

    return 0;
}

void display(const trc::TextureData& tex, const float maxDuration)
{
    auto torch = trc::initFull(
        {},
        trc::InstanceCreateInfo{ .enableRayTracing=false },
        trc::WindowCreateInfo{
            .size=tex.size,
            .title="Texture preview",
        }
    );

    // Set up the camera
    trc::Camera camera;
    camera.makeOrthogonal(-0.5f, 0.5f, -0.5f, 0.5f, -10.0f, 10.0f);
    camera.translateZ(-5.0f);

    // Set up the scene
    trc::SimpleMaterialData mat{
        .emissive=true,
        .albedoTexture=torch->getAssetManager().create(tex),
    };

    trc::Scene scene;
    auto drawable = scene.makeDrawable(trc::DrawableCreateInfo{
        .geo=torch->getAssetManager().create(trc::makePlaneGeo()),
        .mat=torch->getAssetManager().create(trc::makeMaterial(mat)),
        .rasterized=true,
        .rayTraced=false,
    });

    // Rotate the plane so that it faces the viewer
    drawable->rotateX(glm::half_pi<float>());

    trc::Timer timer;
    while (!torch->getWindow().isPressed(trc::Key::escape))
    {
        trc::pollEvents();

        torch->drawFrame(camera, scene);

        const float seconds = timer.duration() * 0.001f;
        if (seconds >= maxDuration) {
            break;
        }
    }

    trc::terminate();
}
