#include "trc/core/RenderPassRegistry.h"

#include <trc_util/TypeUtils.h>



auto trc::resolveRenderPass(
    const std::variant<RenderPassName, RenderPassCompatInfo>& def,
    const RenderPassRegistry& reg)
    -> RenderPassCompatInfo
{
    return std::visit(util::VariantVisitor{
        [&](const RenderPassName& ref){ return reg.getRenderPass(ref); },
        [](const RenderPassCompatInfo& def){ return def; },
    }, def);
}

void trc::RenderPassRegistry::addRenderPass(
    const RenderPassName& name,
    vk::RenderPass renderPass,
    ui32 subPass)
{
    addRenderPass(name, [=]{ return RenderPassInfo{ renderPass, subPass }; });
}

void trc::RenderPassRegistry::addRenderPass(const RenderPassName& name, DynamicRenderingInfo info)
{
    addRenderPass(name, [=]{ return info; });
}

void trc::RenderPassRegistry::addRenderPass(
    const RenderPassName& name,
    const RenderPassGetter& getter)
{
    auto [it, success] = renderPasses.try_emplace(name, getter);

    if (!success)
    {
        throw Exception(
            "[In RenderPassRegistry::addRenderPass]: "
            "Render pass with name " + it->first + " is already registered!"
        );
    }
}

auto trc::RenderPassRegistry::getRenderPass(const RenderPassName& name) const
    -> RenderPassCompatInfo
{
    auto it = renderPasses.find(name);
    if (it != renderPasses.end()) {
        return it->second();
    }

    throw Exception(
        "[In RenderPassRegistry::getRenderPass]: "
        "No render pass with the name \"" + name + "\" has been defined."
    );
}
