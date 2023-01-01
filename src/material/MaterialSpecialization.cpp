#include "trc/material/MaterialSpecialization.h"

#include "trc/drawable/DefaultDrawable.h"
#include "trc/material/VertexShader.h"



namespace trc
{

MaterialKey::MaterialKey(MaterialSpecializationInfo info)
{
    if (info.animated) {
        flags |= Flags::Animated::eTrue;
    }
}

bool MaterialKey::operator==(const MaterialKey& rhs) const
{
    return flags.toIndex() == rhs.flags.toIndex();
}



auto makeMaterialSpecialization(
    ShaderModule fragmentModule,
    const MaterialKey& specialization)
    -> std::unordered_map<vk::ShaderStageFlagBits, ShaderModule>
{
    const bool animated = specialization.flags.has(MaterialKey::Flags::Animated::eTrue);
    ShaderModule vertexModule = VertexModule{ animated }.build(fragmentModule);

    return {
        { vk::ShaderStageFlagBits::eVertex,   std::move(vertexModule) },
        { vk::ShaderStageFlagBits::eFragment, std::move(fragmentModule) },
    };
}

} // namespace trc
