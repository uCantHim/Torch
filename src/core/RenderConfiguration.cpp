#include "RenderConfiguration.h"



void trc::RenderPassRegistry::addRenderPass(RenderPassName name, RenderPassGetter getter)
{
    auto [it, success] = renderPasses.try_emplace(std::move(name.identifier), std::move(getter));

    if (!success)
    {
        throw Exception(
            "[In RenderPassRegistry::addRenderPass]: "
            "Render pass with name " + it->first + " is already defined!"
        );
    }
}

auto trc::RenderPassRegistry::getRenderPass(const RenderPassName& name) const
    -> RenderPassDefinition
{
    auto it = renderPasses.find(name.identifier);
    if (it != renderPasses.end()) {
        return it->second();
    }

    throw Exception(
        "[In RenderPassRegistry::getRenderPass]: "
        "No render pass with the name \"" + name.identifier + "\" has been defined."
    );
}



void trc::DescriptorRegistry::addDescriptor(
    DescriptorName name,
    const DescriptorProviderInterface& provider)
{
    auto [it, success] = descriptorProviders.try_emplace(std::move(name.identifier), &provider);

    if (!success)
    {
        throw Exception(
            "[In DescriptorRegistry::addDescriptor]: "
            "Descriptor with name " + it->first + " is already defined!"
        );
    }
}

auto trc::DescriptorRegistry::getDescriptor(const DescriptorName& name) const
    -> const DescriptorProviderInterface&
{
    auto it = descriptorProviders.find(name.identifier);
    if (it != descriptorProviders.end()) {
        return *it->second;
    }

    throw Exception(
        "[In DescriptorRegistry::getDescriptor]: "
        "No descriptor with the name \"" + name.identifier + "\" has been defined."
    );
}



trc::RenderConfig::RenderConfig(RenderLayout layout)
    :
    layout(std::move(layout))
{
}

auto trc::RenderConfig::getLayout() -> RenderLayout&
{
    return layout;
}
