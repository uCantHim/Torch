#include "DescriptorSetUtils.h"



auto trc::DescriptorSetLayoutBuilder::addFlag(vk::DescriptorSetLayoutCreateFlagBits flags)
    -> Self&
{
    this->layoutFlags |= flags;
    return *this;
}

auto trc::DescriptorSetLayoutBuilder::addBinding(
    vk::DescriptorType type,
    ui32 count,
    vk::ShaderStageFlags stages,
    vk::DescriptorBindingFlags flags) -> Self&
{
    bindings.emplace_back(bindings.size(), type, count, stages);
    bindingFlags.emplace_back(flags);
    return *this;
}

auto trc::DescriptorSetLayoutBuilder::getBindingCount() const -> size_t
{
    return bindings.size();
}

auto trc::DescriptorSetLayoutBuilder::build(const vkb::Device& device)
    -> vk::UniqueDescriptorSetLayout
{
    // VK_EXT_descriptor_indexing is included in 1.2
    vk::StructureChain chain{
        vk::DescriptorSetLayoutCreateInfo(layoutFlags, bindings),
        vk::DescriptorSetLayoutBindingFlagsCreateInfo(bindingFlags)
    };

    return device->createDescriptorSetLayoutUnique(chain.get());
}
