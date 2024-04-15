#include "trc/core/ResourceConfig.h"



namespace trc
{

ResourceStorage::ResourceStorage(s_ptr<ResourceConfig> config, s_ptr<PipelineStorage> pipelines)
    :
    resourceConfig(config),
    pipelines(std::move(pipelines)),
    descriptors(config)
{
}

auto ResourceStorage::getResourceConfig() -> ResourceConfig&
{
    return *resourceConfig;
}

auto ResourceStorage::getResourceConfig() const -> const ResourceConfig&
{
    return *resourceConfig;
}

auto ResourceStorage::getPipeline(Pipeline::ID id) -> Pipeline&
{
    return pipelines->get(id);
}

auto ResourceStorage::getDescriptor(DescriptorID id) const noexcept
    -> s_ptr<const DescriptorProviderInterface>
{
    return descriptors.getDescriptor(id);
}

void ResourceStorage::provideDescriptor(
    const DescriptorName& descName,
    s_ptr<const DescriptorProviderInterface> provider)
{
    descriptors.provideDescriptor(descName, std::move(provider));
}

} // namespace trc
