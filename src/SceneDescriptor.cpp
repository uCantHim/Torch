#include "SceneDescriptor.h"

#include "Scene.h"



vkb::StaticInit trc::SceneDescriptor::_init{
    []() {
        // Create the static descriptor set layout
        std::vector<vk::DescriptorSetLayoutBinding> layoutBindings{
            // Light buffer
            { 0, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eFragment },
            // Picking buffer
            { 1, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eFragment },
        };
        std::vector<vk::DescriptorBindingFlags> layoutFlags{
            vk::DescriptorBindingFlagBits::eUpdateAfterBind, // light buffer
            vk::DescriptorBindingFlagBits::eUpdateAfterBind, // picking buffer
        };

        descLayout = vkb::getDevice()->createDescriptorSetLayoutUnique(
            vk::DescriptorSetLayoutCreateInfo(
                vk::DescriptorSetLayoutCreateFlagBits::eUpdateAfterBindPool, layoutBindings
            )
        );
    },
    []() {
        descLayout.reset();
    }
};



trc::SceneDescriptor::SceneDescriptor(const Scene& scene)
{
    createDescriptors(scene);
}

auto trc::SceneDescriptor::getDescSet() const noexcept -> vk::DescriptorSet
{
    return *descSet;
}

auto trc::SceneDescriptor::getDescLayout() noexcept -> vk::DescriptorSetLayout
{
    return *descLayout;
}

void trc::SceneDescriptor::createDescriptors(const Scene& scene)
{
    descPool = vkb::getDevice()->createDescriptorPoolUnique(vk::DescriptorPoolCreateInfo(
        vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet
            | vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind,
        1, // max num sets
        std::vector<vk::DescriptorPoolSize>{
            { vk::DescriptorType::eStorageBuffer, 2 },
        }
    ));

    descSet = std::move(vkb::getDevice()->allocateDescriptorSetsUnique(
        vk::DescriptorSetAllocateInfo(*descPool, 1, &*descLayout)
    )[0]);

    // Write descriptor set
    vk::DescriptorBufferInfo lightBuffer(scene.getLightBuffer(), 0, VK_WHOLE_SIZE);
    vk::DescriptorBufferInfo pickingBuffer(scene.getPickingBuffer(), 0, VK_WHOLE_SIZE);

    std::vector<vk::WriteDescriptorSet> writes = {
        { *descSet, 0, 0, 1, vk::DescriptorType::eStorageBuffer, {}, &lightBuffer },
        { *descSet, 1, 0, 1, vk::DescriptorType::eStorageBuffer, {}, &pickingBuffer },
    };
    vkb::getDevice()->updateDescriptorSets(writes, {});
}
