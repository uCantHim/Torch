#include "trc/core/ResourceConfig.h"



namespace trc
{

ResourceStorage::ResourceStorage(const ResourceConfig* config, s_ptr<PipelineStorage> pipelines)
    :
    pipelines(std::move(pipelines)),
    descriptors(config)
{
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

} // namespace trc
