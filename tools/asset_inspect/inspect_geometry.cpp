#include <filesystem>

#include <trc/Torch.h>
#include <argparse/argparse.hpp>

constexpr auto kInvalidUsageExitcode{ 64 };

void printInfo(const trc::GeometryData& geo);
void display(const trc::GeometryData& geo, float maxDuration);

int main(int argc, const char* argv[])
{
    argparse::ArgumentParser program;
    program.add_description("Print some information about a geometry and display"
                            " it in a preview window.");

    program.add_argument("file")
        .help("The Torch geometry file to inspect.");

    program.add_argument("--no-text")
        .default_value(false)
        .implicit_value(true)
        .help("Don't print information text to stdout.");
    program.add_argument("--no-display")
        .default_value(false)
        .implicit_value(true)
        .help("Don't display the geometry graphically.");
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

    trc::GeometryData geo;

    // Try to open file
    const auto inputFile = program.get("file");
    if (!fs::is_regular_file(inputFile))
    {
        std::cout << "Error: " << inputFile << " is not a regular file. Exiting.\n";
        exit(1);
    }

    std::ifstream file(inputFile, std::ios::binary);
    if (!file.is_open())
    {
        std::cout << "Error: Unable to open file " << inputFile << ". Exiting.\n";
        exit(1);
    }

    // Try to parse geometry from file
    try {
        geo.deserialize(file);
    }
    catch (const std::exception& err) {
        std::cout << "Error: Unable to parse geometry file " << inputFile << ": "
                  << err.what() << ". Exiting.\n";
        exit(1);
    }

    // Process geometry with the applicable output methods
    if (!program.get<bool>("--no-text")) {
        printInfo(geo);
    }
    if (!program.get<bool>("--no-display"))
    {
        const auto maxDuration = program.present<float>("--display-duration");
        display(geo, maxDuration.value_or(std::numeric_limits<float>::max()));
    }

    return 0;
}



void printInfo(const trc::GeometryData& geo)
{
    std::cout << "Vertices: " << geo.indices.size() << "\n";
    std::cout << "Bone information: " << std::boolalpha << !geo.skeletalVertices.empty() << "\n";
    std::cout << "Rig: " << !geo.rig.empty() << "\n";
}

auto calcRadius(const trc::GeometryData& geo) -> float
{
    glm::vec3 r{ 0.0f };
    for (const auto& v : geo.vertices) {
        r = glm::max(r, glm::abs(v.position));
    }

    return glm::max(glm::max(r.x, r.y), r.z);
}

void display(const trc::GeometryData& geo, float maxDuration)
{
    auto torch = trc::initFull(
        {},
        trc::InstanceCreateInfo{ .enableRayTracing=false },
        trc::WindowCreateInfo{
            .size{ 500, 800 },
            .title{ "Geometry preview" }
        });

    trc::Camera camera;
    camera.makePerspective(torch->getWindow().getAspectRatio(), 45.0f, 0.1f, 100.0f);
    camera.lookAt(trc::vec3(0, 3, 5), trc::vec3(0, 1, 0), trc::vec3(0, 1, 0));
    trc::on<trc::SwapchainResizeEvent>([&](auto&& e){
        camera.setAspect(e.swapchain->getAspectRatio());
    });

    trc::Scene scene;
    trc::SimpleMaterialData material{
        .color=trc::vec4(0.7f, 0.7f, 0.7f, 1.0f),
        .roughness=0.8f,
        .emissive=false,
    };

    auto drawable = scene.makeDrawable(trc::DrawableCreateInfo{
        .geo=torch->getAssetManager().create(geo),
        .mat=torch->getAssetManager().create(trc::makeMaterial(material)),
        .rasterized=true,
        .rayTraced=false
    });
    drawable->setScale(2.5f / calcRadius(geo));
    scene.getLights().makeSunLight(trc::vec3(1.0f), trc::vec3(-1, -1, 0), 0.7f);

    trc::Timer timer;
    while (!torch->getWindow().isPressed(trc::Key::escape))
    {
        trc::pollEvents();

        const float seconds = (timer.duration() * 0.001f);
        drawable->setRotation(seconds * glm::quarter_pi<float>(), glm::vec3(0, 1, 0));
        torch->drawFrame(camera, scene);

        if (seconds >= maxDuration) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    trc::terminate();
}
