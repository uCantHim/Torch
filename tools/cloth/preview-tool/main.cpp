#include <chrono>
#include <filesystem>

#include <argparse/argparse.hpp>
#include <cloth/cloth.h>
#include <cloth/torch_impl.h>
#include <trc/Torch.h>
#include <trc/base/event/Event.h>

namespace fs = std::filesystem;
using namespace trc::basic_types;

class Display
{
public:
    explicit Display(const fs::path& assetDir)
        :
        torch(trc::initFull({ .plugins{}, .assetStorageDir=assetDir, })),
        camera(std::make_shared<trc::Camera>()),
        scene(std::make_shared<trc::Scene>()),
        vp(torch->makeFullscreenViewport(camera, scene)),
        light(scene->getLights().makeSunLight(vec3(1.0f), vec3(1, -1, -1), 0.4f)),
        mat(torch->getAssetManager().create(trc::makeMaterial(
            trc::SimpleMaterialData{ .color{ 1, 1, 1 } }
        ))),
        geo(torch->getAssetManager().create(trc::makeSphereGeo(64, 32)))
    {
        trc::on<trc::WindowResizeEvent>([&](auto&& e) {
            camera->setAspect(e.swapchain->getAspectRatio());
        });
        camera->makePerspective(torch->getWindow().getAspectRatio(), 45.0f, 0.1f, 50.0f);
        camera->lookAt({ 0, 0, 4 }, { 0, 0, 0 }, { 0, 1, 0 });

        updatePreviewObject();
    }

    void update()
    {
        torch->drawFrame(vp);
    }

    auto getCamera() -> trc::Camera& {
        return *camera;
    }
    auto getWindow() -> trc::Window& {
        return torch->getWindow();
    }

    void setMaterial(const trc::MaterialData& data)
    {
        torch->getAssetManager().destroy(mat);
        mat = torch->getAssetManager().create(data);
        updatePreviewObject();
    }

    void setGeometry(const trc::GeometryData& data)
    {
        torch->getAssetManager().destroy(geo);
        geo = torch->getAssetManager().create(data);
        updatePreviewObject();
    }

private:
    void updatePreviewObject()
    {
        materialObj.reset();
        materialObj = scene->makeDrawable({ .geo=geo, .mat=mat, });
    }

    u_ptr<trc::TorchStack> torch;

    s_ptr<trc::Camera> camera;
    s_ptr<trc::Scene> scene;
    trc::ViewportHandle vp;

    trc::SunLight light;

    trc::MaterialID mat;
    trc::GeometryID geo;
    trc::Drawable materialObj;
};

constexpr auto kFileCheckInterval = std::chrono::milliseconds(50);

void run(const fs::path& clothFile, const fs::path& assetDir);

int main(int argc, const char** argv)
{
    argparse::ArgumentParser prog;
    prog.add_argument("file")
        .help("Cloth file to watch");
    prog.add_argument("--asset-dir")
        .default_value(".")
        .help("Directory from where to load assets");
    try {
        prog.parse_args(argc, argv);
    }
    catch (const std::exception& err) {
        std::cout << prog;
        exit(1);
    }

    run(prog.get("file"), prog.get("asset-dir"));
    trc::terminate();

    return 0;
}

void run(const fs::path& clothFile, const fs::path& assetDir)
{
    bool needsRedraw{ true };

    Display display{ assetDir };
    display.update();
    trc::on<trc::WindowResizeEvent>([&](auto&&) {
        needsRedraw = true;
    });

    auto lastCheckTime = fs::last_write_time(clothFile);
    while (display.getWindow().isOpen() && !display.getWindow().isPressed(trc::Key::escape))
    {
        trc::pollEvents();
        std::this_thread::sleep_for(kFileCheckInterval);

        if (display.getWindow().isPressed(trc::Key::f5)) {
            needsRedraw = true;
        }

        if (fs::is_regular_file(clothFile)
            && fs::last_write_time(clothFile) >= lastCheckTime)
        {
            std::ifstream file{ clothFile };
            cloth::TorchImpl clothImpl;
            auto res = cloth::compileShader(file, clothImpl);

            if (res) {
                auto mat = trc::makeMaterial({
                    .fragmentModule=res.value().shaderModule,
                    .transparent=false,
                });
                display.setMaterial(mat);
            }

            needsRedraw = true;
            lastCheckTime = fs::file_time_type::clock::now();
        }

        if (needsRedraw)
        {
            display.update();
            needsRedraw = false;
        }
    }
}
