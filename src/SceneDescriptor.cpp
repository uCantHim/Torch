#include "SceneDescriptor.h"

#include "Scene.h"



trc::SceneDescriptor::SceneDescriptor()
{
    createDescriptors();
}

auto trc::SceneDescriptor::getProvider() const noexcept -> const DescriptorProviderInterface&
{
    return provider;
}

void trc::SceneDescriptor::updateActiveScene(const Scene& scene) const noexcept
{
    vk::DescriptorBufferInfo lightBuffer(scene.getLightBuffer(), 0, VK_WHOLE_SIZE);
    vk::DescriptorBufferInfo pickingBuffer(scene.getPickingBuffer(), 0, VK_WHOLE_SIZE);

    std::vector<vk::WriteDescriptorSet> writes = {
        { *descSet, 0, 0, 1, vk::DescriptorType::eStorageBuffer, {}, &lightBuffer },
        { *descSet, 1, 0, 1, vk::DescriptorType::eStorageBuffer, {}, &pickingBuffer },
    };
    vkb::getDevice()->updateDescriptorSets(writes, {});
}

void trc::SceneDescriptor::createDescriptors()
{
    descPool = vkb::getDevice()->createDescriptorPoolUnique(vk::DescriptorPoolCreateInfo(
        vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet
        | vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind,
        1,
        std::vector<vk::DescriptorPoolSize>{
            { vk::DescriptorType::eStorageBuffer, 1 }, // Light buffer
            { vk::DescriptorType::eStorageBuffer, 1 }, // Picking buffer
        }
    ));

    std::vector<vk::DescriptorSetLayoutBinding> layoutBindings{
        { 0, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eFragment },
        { 1, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eFragment },
    };
    std::vector<vk::DescriptorBindingFlags> layoutFlags{
        vk::DescriptorBindingFlagBits::eUpdateAfterBind,
        vk::DescriptorBindingFlagBits::eUpdateAfterBind,
    };
    vk::StructureChain layoutChain
    {
        vk::DescriptorSetLayoutCreateInfo(
            vk::DescriptorSetLayoutCreateFlagBits::eUpdateAfterBindPool, layoutBindings
        ),
        vk::DescriptorSetLayoutBindingFlagsCreateInfo(layoutFlags)
    };
    descLayout = vkb::getDevice()->createDescriptorSetLayoutUnique(
        layoutChain.get<vk::DescriptorSetLayoutCreateInfo>()
    );

    descSet = std::move(vkb::getDevice()->allocateDescriptorSetsUnique(
        vk::DescriptorSetAllocateInfo(*descPool, 1, &*descLayout)
    )[0]);

    provider.setDescriptorSetLayout(*descLayout);
    provider.setDescriptorSet(*descSet);
}
