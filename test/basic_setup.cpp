#include <trc/Torch.h>

int main()
{
    {
        // Initialize Torch
        trc::TorchStack torch = trc::initFull();

        // The things required to render something are
        //   1. A scene
        //   2. A camera
        trc::Scene scene;
        trc::Camera camera;

        // We create a draw configuration that tells the renderer what to
        // draw. We can create this object yourself, but here we let a
        // utility function do it for us.
        trc::DrawConfig drawConf = torch.makeDrawConfig(scene, camera);

        // Main loop
        while (torch.window->getSwapchain().isOpen())
        {
            // Poll system events
            trc::pollEvents();

            // Draw a frame
            torch.drawFrame(drawConf);
        }

        // End of scope, the TorchStack object gets destroyed
    }

    // Call this after you've destroyed all Torch/Vulkan resources.
    trc::terminate();

    return 0;
}
