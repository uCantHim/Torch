#include <filesystem>

#include <argparse/argparse.hpp>
#include <trc/Torch.h>

#include "display_utils.h"
#include "load_utils.h"

constexpr auto kInvalidUsageExitcode{ 64 };

void printInfo(const trc::GeometryData& geo);
void display(const trc::GeometryData& geo, float maxDuration);

constexpr const char* description =
R"(Print some information about a geometry and display it in a preview window.

The preview window supports the following commands via keyboard shortcuts:
 - Rotate the geometry around an axis by 90° [x], [y], [z], [X], [Y], [Z]
 - Zoom in  [-]
 - Zoom out [+]
 - Move camera up/down [k], [j]
 - Move camera left/right [h], [l])";

int main(int argc, const char* argv[])
{
    argparse::ArgumentParser program;
    program.add_description(description);

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

    // Try to open file
    const auto inputFile = program.get("file");
    auto geo = tryLoad<trc::Geometry>(inputFile);
    if (!geo) {
        std::cout << "Error: " << geo.error() << ". Exiting.\n";
        exit(1);
    }

    // Process geometry with the applicable output methods
    if (!program.get<bool>("--no-text")) {
        printInfo(*geo);
    }
    if (!program.get<bool>("--no-display"))
    {
        const auto maxDuration = program.present<float>("--display-duration");
        display(*geo, maxDuration.value_or(std::numeric_limits<float>::max()));
    }

    return 0;
}



void printInfo(const trc::GeometryData& geo)
{
    std::cout << "Vertices: " << geo.indices.size() << "\n";
    std::cout << "Bone information: " << std::boolalpha << !geo.skeletalVertices.empty() << "\n";
    std::cout << "Rig: " << !geo.rig.empty() << "\n";
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
    trc::Scene scene;

    scene.getLights().makeSunLight(trc::vec3(1.0f), trc::vec3(-1, -1, 0), 0.7f);

    // Create a drawable
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
    trc::Node node;
    node.attach(*drawable);
    scene.getRoot().attach(node);

    // Normalize the geometry's size
    const vec3 geoExtent = calcExtent(geo);
    drawable->setScale(1.0f / maxOf(geoExtent));

    // Center camera on the geometry
    const vec3 camPos{ 0.0f, 1.5f, 2.0f };
    const vec3 camTarget{ 0.0f, 0.6f, 0.0f };
    camera.makePerspective(torch->getWindow().getAspectRatio(), 45.0f, 0.1f, 50.0f);
    camera.lookAt(camPos, camTarget, trc::vec3(0, 1, 0));

    // Set up user interaction
    trc::on<trc::SwapchainResizeEvent>([&](auto&& e){
        camera.setAspect(e.swapchain->getAspectRatio());
    });

    trc::on<trc::CharInputEvent>([&](const trc::CharInputEvent& e) {
        constexpr char rotateX = 'x';
        constexpr char rotateY = 'y';
        constexpr char rotateZ = 'z';
        constexpr char rotateMinusX = 'X';
        constexpr char rotateMinusY = 'Y';
        constexpr char rotateMinusZ = 'Z';
        constexpr char zoomIn = '+';
        constexpr char zoomOut = '-';
        constexpr char moveUp    = { 'k' };
        constexpr char moveDown  = { 'j' };
        constexpr char moveLeft  = { 'h' };
        constexpr char moveRight = { 'l' };

        static const vec3 zoom = glm::normalize(camTarget - camPos) * 0.1f;
        static const float movement = 0.1f;

        // State
        static int zoomLevel{ 0 };
        static ivec3 movementLevel{ 0, 0, 0 };

        switch (e.character)
        {
        case rotateX:      drawable->rotateX(glm::half_pi<float>()); break;
        case rotateY:      drawable->rotateY(glm::half_pi<float>()); break;
        case rotateZ:      drawable->rotateZ(glm::half_pi<float>()); break;
        case rotateMinusX: drawable->rotateX(-glm::half_pi<float>()); break;
        case rotateMinusY: drawable->rotateY(-glm::half_pi<float>()); break;
        case rotateMinusZ: drawable->rotateZ(-glm::half_pi<float>()); break;
        case zoomIn:  ++zoomLevel; break;
        case zoomOut: --zoomLevel; break;
        case moveUp:    ++movementLevel.y; break;
        case moveDown:  --movementLevel.y; break;
        case moveLeft:  --movementLevel.x; break;
        case moveRight: ++movementLevel.x; break;
        default: return;
        }

        // Recalculate camera position
        camera.lookAt(camPos + zoom * static_cast<float>(zoomLevel),
                      camTarget,
                      vec3(0, 1, 0));
        camera.translate(vec3{-movementLevel} * movement);
    });

    trc::Timer timer;
    float totalTime{ 0.0f };
    while (!torch->getWindow().isPressed(trc::Key::escape))
    {
        constexpr float rotationSpeed = glm::quarter_pi<float>() * 0.5f;

        trc::pollEvents();

        const float timeDelta = timer.reset();
        scene.update(timeDelta);

        const float seconds = timeDelta * 0.001f;
        node.rotateY(seconds * rotationSpeed);
        torch->drawFrame(camera, scene);

        if ((totalTime += seconds) >= maxDuration) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    trc::terminate();
}
