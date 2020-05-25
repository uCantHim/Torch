// #pragma comment(lib, "vulkan-1.lib")
// #pragma comment(lib, "glfw3.lib")

#include "VulkanApp.h"

int main()
{
    vkb::vulkanInit({});

    VulkanApp app;
    app.run();

    vkb::vulkanTerminate();
}
