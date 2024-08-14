#include <trc/Torch.h>
#include <trc/drawable/DrawableScene.h>

int main()
{
    // Initialize Torch.
    trc::init();

    // Create a default Torch setup.
    trc::TorchStack torch;

    // The things required to render something are
    //   1. A scene
    //   2. A camera
    auto scene = std::make_shared<trc::DrawableScene>();
    auto camera = std::make_shared<trc::Camera>();
    camera->makeOrthogonal(-1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f);

    // Create a triangle geometry and a blue-colored material.
    auto& assets = torch.getAssetManager();
    trc::GeometryID geo = assets.create(trc::makeTriangleGeo());
    trc::MaterialID mat = assets.create(trc::makeMaterial(trc::SimpleMaterialData{
        .color={ 0.392f, 0.624f, 0.82f },
        .emissive=true,  // So we don't need any lighting yet.
    }));

    // Create a drawable object with our new assets.
    auto myDrawable = scene->makeDrawable({ geo, mat });

    // Torch draws scenes to viewports. Create a viewport.
    auto vp = torch.makeFullscreenViewport(camera, scene);

    // Main loop
    while (torch.getWindow().isOpen())
    {
        // Poll system events (required to check whether the window should be closed)
        trc::pollEvents();

        // Draw a frame
        torch.drawFrame(vp);
    }

    // Call this after you've destroyed all Torch/Vulkan resources.
    trc::terminate();

    return 0;
}
