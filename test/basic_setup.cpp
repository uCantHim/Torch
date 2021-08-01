#include <trc/Torch.h>

int main()
{
    {
        auto torch = trc::initDefault();

        // The things required to render something are
        //   1. A scene
        //   2. A camera
        //   3. A render configuration, which defines a concrete rendering
        //      implementation - for example a deferred renderer.
        // The scene is created as a unique_ptr instead of value here so we can
        // easily delete it at the end. That's not necessary for the camera
        // since it doesn't allocate any Vulkan resources.
        trc::Scene scene{ *torch.instance };
        trc::Camera camera;

        trc::DrawConfig drawConf{
            .scene=&scene,
            .camera=&camera,
            .renderConfig=torch.renderConfig.get(),
            .renderAreas={
                trc::RenderArea{
                    .viewport=vk::Viewport(0, 0, 100, 100),
                    .scissor=vk::Rect2D({ 0, 0 }, { 100, 100 })
                }
            }
        };

        // Main loop
        while (torch.window->getSwapchain().isOpen())
        {
            // Poll system events
            vkb::pollEvents();

            // Use the renderer to draw the scene
            //renderer->drawFrame(*scene, camera);
            torch.window->drawFrame(drawConf);
        }

        torch.instance->getDevice()->waitIdle();

        trc::AssetRegistry::reset();
    }

    // Call this after you've destroyed all Torch/Vulkan resources.
    trc::terminate();

    return 0;
}
