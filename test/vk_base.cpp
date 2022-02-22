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

        auto surface = vkb::makeSurface(*instance, {});
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

        vkb::Swapchain swapchain(device, vkb::makeSurface(*instance, {}));
        log("Swapchain created");


        std::vector<vk::DescriptorPoolSize> poolSizes{
            vk::DescriptorPoolSize(vk::DescriptorType::eSampledImage, 2),
            vk::DescriptorPoolSize(vk::DescriptorType::eSampledImage, 4),
        };
        auto pool = device->createDescriptorPoolUnique({ vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet, 1, poolSizes });

        std::vector<vk::DescriptorSetLayoutBinding> bindings{
            vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eSampledImage, 6, vk::ShaderStageFlagBits::eFragment),
        };
        auto layout = device->createDescriptorSetLayoutUnique({ {}, bindings });

        auto set = device->allocateDescriptorSetsUnique({ *pool, *layout });
    }

    vkb::terminate();

    return 0;
}
