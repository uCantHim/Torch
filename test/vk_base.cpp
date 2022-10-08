#include <memory>
#include <iostream>

#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>

#include <trc/Torch.h>

void log(auto v)
{
    std::cout << "--- (TEST): " << v << "\n";
}

int main()
{
    trc::init();

    {
        trc::VulkanInstance instance;
        log("Instance created");

        std::unique_ptr<trc::PhysicalDevice> phys;
        try {
            trc::Surface surface(*instance, {});
            log("Surface and window created");

            phys = std::make_unique<trc::PhysicalDevice>(*instance, surface.getVulkanSurface());
            log("Optimal physical device found");
        }
        catch (const std::exception& err) {
            log("Physical device creation failed: " + std::string(err.what()));
            return 1;
        }
        log("Surface and Window destroyed");

        trc::Device device(*phys);
        log("Logical device created");

        trc::Swapchain swapchain(device, trc::Surface(*instance, {}));
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

    trc::terminate();

    return 0;
}
