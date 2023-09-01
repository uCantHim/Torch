#include "trc/DescriptorSetUtils.h"

#include <unordered_map>



auto trc::DescriptorSetLayoutBuilder::addFlag(vk::DescriptorSetLayoutCreateFlagBits flags)
    -> Self&
{
    switch (flags)
    {
    case vk::DescriptorSetLayoutCreateFlagBits::eHostOnlyPoolEXT:
        this->poolFlags |= vk::DescriptorPoolCreateFlagBits::eHostOnlyEXT;
        break;
    case vk::DescriptorSetLayoutCreateFlagBits::eUpdateAfterBindPool:
        this->poolFlags |= vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind;
        break;
    default:
        break;
    }

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

auto trc::DescriptorSetLayoutBuilder::build(const Device& device)
    -> vk::UniqueDescriptorSetLayout
{
    // VK_EXT_descriptor_indexing is included in 1.2
    vk::StructureChain chain{
        vk::DescriptorSetLayoutCreateInfo(layoutFlags, bindings),
        vk::DescriptorSetLayoutBindingFlagsCreateInfo(bindingFlags)
    };

    return device->createDescriptorSetLayoutUnique(chain.get());
}

auto trc::DescriptorSetLayoutBuilder::buildPool(
    const Device& device,
    const ui32 maxSets,
    const vk::DescriptorPoolCreateFlags flags)
    -> vk::UniqueDescriptorPool
{
    std::unordered_map<vk::DescriptorType, ui32> descTypes;
    for (const auto& binding : bindings)
    {
        auto [it, _] = descTypes.try_emplace(binding.descriptorType, 0);
        it->second += binding.descriptorCount;
    }

    std::vector<vk::DescriptorPoolSize> poolSizes;
    for (const auto& [type, count] : descTypes) {
        poolSizes.emplace_back(type, count);
    }

    return device->createDescriptorPoolUnique(vk::DescriptorPoolCreateInfo{
        poolFlags | flags,
        maxSets,
        poolSizes
    });
}
