#include "trc/core/DescriptorRegistry.h"

#include <cassert>

#include <trc_util/Assert.h>
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



//////////////////////////////
//    Descriptor storage    //
//////////////////////////////

trc::DescriptorStorage::DescriptorStorage(s_ptr<const DescriptorRegistry> registry)
    :
    registry(registry)
{
}

void trc::DescriptorStorage::provideDescriptor(
    const DescriptorName& name,
    s_ptr<const DescriptorProviderInterface> provider)
{
    assert_arg(provider != nullptr);
    assert(provider != nullptr);

    try {
        const DescriptorID id = registry->getDescriptorID(name);
        descriptorProviders[id] = std::move(provider);
    }
    catch (const Exception&) {
        throw Exception(
            "[In DescriptorStorage::provideDescriptor]:"
            " Unable to define a resource for descriptor \"" + name.identifier + "\" as no"
            " descriptor with that name is defined at the parent DescriptorRegistry."
        );
    }
}

auto trc::DescriptorStorage::getDescriptor(const DescriptorName& descName) const noexcept
    -> s_ptr<const DescriptorProviderInterface>
{
    try {
        const auto id = registry->getDescriptorID(descName);
        return getDescriptor(id);
    }
    catch (const Exception&) {
        return nullptr;
    }
}

auto trc::DescriptorStorage::getDescriptor(DescriptorID id) const noexcept
    -> s_ptr<const DescriptorProviderInterface>
{
    if (id >= descriptorProviders.size()) {
        return nullptr;
    }
    return descriptorProviders[id];
}
