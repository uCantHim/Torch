#include "trc/core/ResourceConfig.h"

#include <trc_util/Assert.h>



namespace trc
{

ResourceStorage::ResourceStorage(s_ptr<ResourceConfig> config, s_ptr<PipelineStorage> pipelines)
    :
    resourceConfig(config),
    pipelines(std::move(pipelines)),
    descriptors(config)
{
}

ResourceStorage::ResourceStorage(s_ptr<ResourceStorage> _parent)
    :
    parent([&]{
        assert(_parent != nullptr);
        return _parent;
    }()),
    resourceConfig(_parent->resourceConfig),
    pipelines(_parent->pipelines),
    descriptors(_parent->descriptors)
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
    try {
        return pipelines->get(id);
    }
    catch (const std::exception&)
    {
        if (parent != nullptr) {
            return parent->getPipeline(id);
        }
    }

    throw std::out_of_range("");
}

auto ResourceStorage::getDescriptor(DescriptorID id) const noexcept
    -> s_ptr<const DescriptorProviderInterface>
{
    if (auto desc = descriptors.getDescriptor(id)) {
        return desc;
    }

    // Try to fall back on a parent resource storage
    if (parent != nullptr) {
        return parent->getDescriptor(id);
    }

    // No resource found
    return nullptr;
}

void ResourceStorage::provideDescriptor(
    const DescriptorName& descName,
    s_ptr<const DescriptorProviderInterface> provider)
{
    descriptors.provideDescriptor(descName, std::move(provider));
}

auto ResourceStorage::derive(s_ptr<ResourceStorage> parent) -> u_ptr<ResourceStorage>
{
    assert_arg(parent != nullptr);
    return u_ptr<ResourceStorage>{ new ResourceStorage{ std::move(parent) } };  // private ctor :(
}

} // namespace trc
