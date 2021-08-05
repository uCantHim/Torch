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

        auto extent = torch.window->getSwapchain().getImageExtent();
        extent.width *= 0.5;
        extent.height *= 0.5;

        trc::DrawConfig drawConf{
            .scene=&scene,
            .camera=&camera,
            .renderConfig=torch.renderConfig.get(),
            .renderAreas={
                trc::RenderArea{
                    .viewport=vk::Viewport(0, 0, extent.width, extent.height),
                    .scissor=vk::Rect2D({ 0, 0 }, extent)
                }
            }
        };

        vkb::on<vkb::SwapchainResizeEvent>([&](const vkb::SwapchainResizeEvent& e) {
            auto extent = e.swapchain->getImageExtent();
            auto& area = drawConf.renderAreas[0];
            area.viewport = vk::Viewport(0, 0, extent.width, extent.height),
            area.scissor = vk::Rect2D({ 0, 0 }, extent);
        });

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
    }

    // Call this after you've destroyed all Torch/Vulkan resources.
    trc::terminate();

    return 0;
}
