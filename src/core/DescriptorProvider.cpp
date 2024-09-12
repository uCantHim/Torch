#include "trc/core/DescriptorProvider.h"



trc::DescriptorProvider::DescriptorProvider(vk::DescriptorSet set)
    :
    set(set)
{
}

void trc::DescriptorProvider::bindDescriptorSet(
    vk::CommandBuffer cmdBuf,
    vk::PipelineBindPoint bindPoint,
    vk::PipelineLayout pipelineLayout,
    ui32 setIndex) const
{
    cmdBuf.bindDescriptorSets(bindPoint, pipelineLayout, setIndex, set, {});
}

void trc::DescriptorProvider::setDescriptorSet(vk::DescriptorSet newSet)
{
    set = newSet;
}



trc::FrameSpecificDescriptorProvider::FrameSpecificDescriptorProvider(
    FrameSpecific<vk::DescriptorSet> set)
    :
    set(std::move(set))
{
}

void trc::FrameSpecificDescriptorProvider::bindDescriptorSet(
    vk::CommandBuffer cmdBuf,
    vk::PipelineBindPoint bindPoint,
    vk::PipelineLayout pipelineLayout,
    ui32 setIndex) const
{
    cmdBuf.bindDescriptorSets(bindPoint, pipelineLayout, setIndex, *set, {});
}

void trc::FrameSpecificDescriptorProvider::setDescriptorSet(
    FrameSpecific<vk::DescriptorSet> newSet)
{
    set = std::move(newSet);
}
