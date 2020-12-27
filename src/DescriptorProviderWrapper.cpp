#include "DescriptorProviderWrapper.h"



trc::DescriptorProviderWrapper::DescriptorProviderWrapper(
    vk::DescriptorSetLayout staticLayout)
    :
    descLayout(staticLayout)
{
}

auto trc::DescriptorProviderWrapper::getDescriptorSet() const noexcept
    -> vk::DescriptorSet
{
    if (provider != nullptr) {
        return provider->getDescriptorSet();
    }
    return {};
}

auto trc::DescriptorProviderWrapper::getDescriptorSetLayout() const noexcept
    -> vk::DescriptorSetLayout
{
    return descLayout;
}

void trc::DescriptorProviderWrapper::bindDescriptorSet(
    vk::CommandBuffer cmdBuf,
    vk::PipelineBindPoint bindPoint,
    vk::PipelineLayout pipelineLayout,
    ui32 setIndex) const
{
    if (provider != nullptr) {
        provider->bindDescriptorSet(cmdBuf, bindPoint, pipelineLayout, setIndex);
    }
}

void trc::DescriptorProviderWrapper::setWrappedProvider(
    const DescriptorProviderInterface& wrapped) noexcept
{
    setDescLayout(wrapped.getDescriptorSetLayout());
    provider = &wrapped;
}

void trc::DescriptorProviderWrapper::setDescLayout(vk::DescriptorSetLayout layout)
{
    descLayout = layout;
}

auto trc::DescriptorProviderWrapper::operator=(const DescriptorProviderInterface& wrapped) noexcept
    -> DescriptorProviderWrapper&
{
    setWrappedProvider(wrapped);
    return *this;
}
