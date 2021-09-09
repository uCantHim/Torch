#include <memory>
#include <iostream>

#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>

#include <vkb/VulkanBase.h>

void log(auto v)
{
    std::cout << "--- (TEST): " << v << "\n";
}

int main()
{
    vkb::init();

    {
        vkb::VulkanInstance instance;
        log("Instance created");

        auto surface = vkb::createSurface(*instance, {});
        log("Surface and window created");

        std::unique_ptr<vkb::PhysicalDevice> phys;
        try {
            phys = std::make_unique<vkb::PhysicalDevice>(*instance, *surface.surface);
            log("Optimal physical device found");

            surface.surface.reset();
            log("Surface destroyed");
            surface.window.reset();
            log("Window destroyed");
        }
        catch (const std::exception& err) {
            log("Physical device creation failed: " + std::string(err.what()));
            return 1;
        }

        vkb::Device device(*phys);
        log("Logical device created");

        vkb::Swapchain swapchain(device, vkb::createSurface(*instance, {}));
        log("Swapchain created");
    }

    vkb::terminate();

    return 0;
}
