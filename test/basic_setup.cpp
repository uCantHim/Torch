#include <trc/Torch.h>

int main()
{
    // Initialize Torch and create a renderer. Do have to do this before
    // you use any other functionality of Torch.
    auto renderer = trc::init();

    // The two things required to render something are
    //   1. A scene
    //   2. A camera
    // The scene is created as a unique_ptr instead of value here so we can
    // easily delete it at the end. That's not necessary for the camera
    // since it doesn't allocate any Vulkan resources.
    auto scene = std::make_unique<trc::Scene>();
    trc::Camera camera;

    // Main loop
    while (vkb::getSwapchain().isOpen())
    {
        // Poll system events
        vkb::pollEvents();

        // Use the renderer to draw the scene
        renderer->drawFrame(*scene, camera);
    }

    // Destroy the Torch resources
    renderer.reset();
    scene.reset();

    // Call this after you've destroyed all Torch/Vulkan resources.
    trc::terminate();

    return 0;
}
