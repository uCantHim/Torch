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

        std::unique_ptr<vkb::PhysicalDevice> phys;
        try {
            vkb::Surface surface(*instance, {});
            log("Surface and window created");

            phys = std::make_unique<vkb::PhysicalDevice>(*instance, surface.getVulkanSurface());
            log("Optimal physical device found");
        }
        catch (const std::exception& err) {
            log("Physical device creation failed: " + std::string(err.what()));
            return 1;
        }
        log("Surface and Window destroyed");

        vkb::Device device(*phys);
        log("Logical device created");

        vkb::Swapchain swapchain(device, vkb::Surface(*instance, {}));
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
