#include "trc/core/DescriptorRegistry.h"

#include <cassert>

#include <trc_util/Exception.h>

#include "trc/core/DescriptorProvider.h"



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

void trc::DescriptorRegistry::defineDescriptor(
    const DescriptorName& name,
    vk::DescriptorSetLayout layout)
{
    assert(layout != VK_NULL_HANDLE);

    const DescriptorID id = tryInsertName(name);
    if (descriptorSetLayouts[id] != VK_NULL_HANDLE)
    {
        throw Exception(
            "[In DescriptorRegistry::defineDescriptor]: "
            "Descriptor with name " + name.identifier + " is already defined!"
        );
    }
    descriptorSetLayouts[id] = layout;
}

void trc::DescriptorRegistry::provideDescriptor(
    const DescriptorName& name,
    s_ptr<const DescriptorProviderInterface> provider)
{
    try {
        const DescriptorID id = idPerName.at(name.identifier);
        descriptorProviders[id] = std::move(provider);
    }
    catch (const std::out_of_range&) {
        throw Exception(
            "[In DescriptorRegistry::provideDescriptor]: "
            "No descriptor with the name \"" + name.identifier + "\" is defined."
        );
    }
}

auto trc::DescriptorRegistry::getDescriptorLayout(const DescriptorName& name) const
    -> vk::DescriptorSetLayout
{
    try {
        const DescriptorID id = idPerName.at(name.identifier);
        assert(descriptorSetLayouts[id] != VK_NULL_HANDLE);
        return descriptorSetLayouts[id];
    }
    catch (const std::out_of_range&)
    {
        throw Exception(
            "[In DescriptorRegistry::getDescriptorLayout]: "
            "No descriptor with the name \"" + name.identifier + "\" is defined."
        );
    }
}

auto trc::DescriptorRegistry::getDescriptorID(const DescriptorName& name) const -> DescriptorID
{
    try {
        return idPerName.at(name.identifier);
    }
    catch (const std::out_of_range&) {
        throw Exception(
            "[In DescriptorRegistry::getDescriptorID]: "
            "No descriptor with the name \"" + name.identifier + "\" is defined."
        );
    }
}

auto trc::DescriptorRegistry::getDescriptor(DescriptorID id) const
    -> s_ptr<const DescriptorProviderInterface>
{
    assert(id < descriptorProviders.size());

    if (descriptorProviders[id] == nullptr) {
        throw Exception("[In DescriptorRegistry::getDescriptor]: The descriptor with ID #"
                        + std::to_string(id) + " has no associated descriptor provider! Call"
                        " DescriptorRegistry::provideDescriptor before using that descriptor"
                        " in a pipeline bind.");
    }
    return descriptorProviders[id];
}
