#include <trc/Torch.h>

int main()
{
    // Initialize Torch
    trc::init();

    // Create a default Torch setup
    trc::TorchStack torch;

    // The things required to render something are
    //   1. A scene
    //   2. A camera
    trc::Scene scene;
    trc::Camera camera;

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
