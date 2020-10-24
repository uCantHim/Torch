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
            // Shadow matrix buffer
            { 2, vk::DescriptorType::eStorageBuffer, 1,
              vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
            // Shadow maps
            { 3, vk::DescriptorType::eCombinedImageSampler, 128,
              vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
        };
        std::vector<vk::DescriptorBindingFlags> layoutFlags{
            vk::DescriptorBindingFlagBits::eUpdateAfterBind, // light buffer
            vk::DescriptorBindingFlagBits::eUpdateAfterBind, // picking buffer
            vk::DescriptorBindingFlagBits::eUpdateAfterBind, // shadow matrix buffer
            vk::DescriptorBindingFlagBits::eVariableDescriptorCount, // shadow map samplers
        };

        vk::StructureChain layoutChain{
            vk::DescriptorSetLayoutCreateInfo(
                vk::DescriptorSetLayoutCreateFlagBits::eUpdateAfterBindPool, layoutBindings
            ),
            vk::DescriptorSetLayoutBindingFlagsCreateInfo(layoutFlags)
        };
        descLayout = vkb::getDevice()->createDescriptorSetLayoutUnique(
            layoutChain.get<vk::DescriptorSetLayoutCreateInfo>()
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
        1,
        std::vector<vk::DescriptorPoolSize>{
            { vk::DescriptorType::eStorageBuffer, 3 },
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
