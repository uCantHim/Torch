#include "core/DescriptorRegistry.h"

#include <trc_util/Exception.h>

#include "DescriptorProvider.h"



auto trc::DescriptorRegistry::tryInsertName(const DescriptorName& name) -> DescriptorID
{
    auto [it, success] = idPerName.try_emplace(std::move(name.identifier));
    auto& [_, id] = *it;
    if (success)
    {
        // The name is new; generate an ID for it.
        id = DescriptorID(descriptorIdPool.generate());
    }

    return id;
}

auto trc::DescriptorRegistry::addDescriptor(
    DescriptorName name,
    const DescriptorProviderInterface& provider)
    -> DescriptorID
{
    DescriptorID id = tryInsertName(name);

    if (descriptorProviders[id] != nullptr)
    {
        throw Exception(
            "[In DescriptorRegistry::addDescriptor]: "
            "Descriptor with name " + name.identifier + " is already defined!"
        );
    }
    descriptorProviders[id] = &provider;

    return id;
}

auto trc::DescriptorRegistry::getDescriptorLayout(const DescriptorName& name) const
    -> vk::DescriptorSetLayout
{
    try {
        return getDescriptor(getDescriptorID(name)).getDescriptorSetLayout();
    }
    catch (const Exception&)
    {
        throw Exception(
            "[In DescriptorRegistry::getDescriptorLayout]: "
            "No descriptor with the name \"" + name.identifier + "\" has been defined."
        );
    }
}

auto trc::DescriptorRegistry::getDescriptorID(const DescriptorName& name) -> DescriptorID
{
    auto it = idPerName.find(name.identifier);
    if (it != idPerName.end())
    {
        return it->second;
    }

    throw Exception(
        "[In DescriptorRegistry::getDescriptor]: "
        "No descriptor with the name \"" + name.identifier + "\" has been defined."
    );
}

auto trc::DescriptorRegistry::getDescriptor(DescriptorID id) const
    -> const DescriptorProviderInterface&
{
    assert(id < descriptorProviders.size());
    assert(descriptorProviders[id] != nullptr);
    return *descriptorProviders[id];
}
