#include "trc/core/PipelineLayout.h"

#include <cassert>

#include "trc/core/DescriptorProvider.h"



trc::PipelineLayout::PipelineLayout(vk::UniquePipelineLayout layout)
    :
    layout(std::move(layout))
{
}

trc::PipelineLayout::PipelineLayout(
    const Device& device,
    const vk::ArrayProxy<const vk::DescriptorSetLayout>& descriptors,
    const vk::ArrayProxy<const vk::PushConstantRange>& pushConstants)
    :
    layout(device->createPipelineLayoutUnique({ {}, descriptors, pushConstants }))
{
}

auto trc::PipelineLayout::operator*() const noexcept -> vk::PipelineLayout
{
    return *layout;
}

void trc::PipelineLayout::bindStaticDescriptorSets(
    vk::CommandBuffer cmdBuf,
    vk::PipelineBindPoint bindPoint) const
{
    for (const auto& [index, provider] : staticDescriptorSets)
    {
        provider->bindDescriptorSet(cmdBuf, bindPoint, *layout, index);
    }
}

void trc::PipelineLayout::bindStaticDescriptorSets(
    vk::CommandBuffer cmdBuf,
    vk::PipelineBindPoint bindPoint,
    const DescriptorRegistry& registry) const
{
    /**
     * I don't know if I have to bind the descriptors in their order of
     * indices (lowest to highest). The specification does not seem very
     * clear to me, so I'll bind them out-of-order for now.
     *
     * https://www.khronos.org/registry/vulkan/specs/1.2-extensions/html/vkspec.html#descriptorsets-compatibility
     */

    for (const auto& [index, provider] : staticDescriptorSets)
    {
        assert(provider != nullptr);
        provider->bindDescriptorSet(cmdBuf, bindPoint, *layout, index);
    }
    for (const auto& [index, id] : dynamicDescriptorSets)
    {
        auto provider = registry.getDescriptor(id);
        assert(provider != nullptr);
        provider->bindDescriptorSet(cmdBuf, bindPoint, *layout, index);
    }
}

void trc::PipelineLayout::bindDefaultPushConstantValues(vk::CommandBuffer cmdBuf) const
{
    for (const auto& [offset, stages, data] : defaultPushConstants)
    {
        cmdBuf.pushConstants(*layout, stages, offset, data.size(), data.data());
    }
}

void trc::PipelineLayout::addStaticDescriptorSet(
    ui32 descriptorIndex,
    s_ptr<const DescriptorProviderInterface> provider) noexcept
{
    staticDescriptorSets.emplace_back(descriptorIndex, provider);
}

void trc::PipelineLayout::addStaticDescriptorSet(ui32 descriptorIndex, DescriptorID id) noexcept
{
    dynamicDescriptorSets.emplace_back(descriptorIndex, id);
}
