#include <trc/GBufferPass.h>
#include <trc/Torch.h>
#include <trc/TorchRenderStages.h>
#include <trc/core/SceneBase.h>
#include <trc/drawable/DefaultDrawable.h>
#include <trc/drawable/DrawableScene.h>

int main()
{
    // Initialize Torch
    trc::init();

    // Create a default Torch setup
    trc::TorchStack torch;

    // The things required to render something are
    //   1. A scene
    //   2. A camera
    trc::DrawableScene scene;
    trc::Camera camera;
    camera.makeOrthogonal(-1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f);

    auto& assets = torch.getAssetManager();

    trc::GeometryID geo = assets.create(trc::makeTriangleGeo());
    trc::MaterialID mat = assets.create(trc::makeMaterial(trc::SimpleMaterialData{
        .color={ 0.392f, 0.624f, 0.82f },
        .emissive=true,
    }));
    auto myDrawable = scene.makeDrawable({ geo, mat });

    // Main loop
    while (torch.getWindow().isOpen())
    {
        // Poll system events
        trc::pollEvents();

        // Draw a frame
        torch.drawFrame(camera, scene);
    }

    // Call this after you've destroyed all Torch/Vulkan resources.
    trc::terminate();

    return 0;
}
