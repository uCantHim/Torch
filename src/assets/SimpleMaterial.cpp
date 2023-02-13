#include "trc/assets/SimpleMaterial.h"

#include "trc/assets/AssetSource.h"
#include "trc/assets/import/InternalFormat.h"
#include "trc/base/Logging.h"
#include "trc/material/CommonShaderFunctions.h"
#include "trc/material/FragmentShader.h"
#include "trc/material/ShaderCapabilityConfig.h"
#include "trc/material/TorchMaterialSettings.h"
#include "trc/ray_tracing/RayPipelineBuilder.h"



namespace trc
{

void SimpleMaterialData::resolveReferences(AssetManager& man)
{
    if (!albedoTexture.empty()) {
        albedoTexture.resolve(man);
    }
    if (!normalTexture.empty()) {
        normalTexture.resolve(man);
    }
}

void SimpleMaterialData::serialize(std::ostream& os) const
{
    serial::SimpleMaterial mat = internal::serializeAssetData(*this);
    mat.SerializeToOstream(&os);
}

void SimpleMaterialData::deserialize(std::istream& is)
{
    serial::SimpleMaterial mat;
    mat.ParseFromIstream(&is);
    *this = internal::deserializeAssetData(mat);
}



auto makeMaterial(const SimpleMaterialData& data) -> MaterialData
{
    ShaderModuleBuilder builder(makeFragmentCapabiltyConfig());

    code::Value color = builder.makeConstant(vec4(data.color, data.opacity));;
    if (!data.albedoTexture.empty())
    {
        color = builder.makeTextureSample(
            TextureReference{ data.albedoTexture },
            builder.makeCapabilityAccess(FragmentCapability::kVertexUV)
        );
    }

    code::Value normal = builder.makeCapabilityAccess(FragmentCapability::kVertexNormal);;
    if (!data.normalTexture.empty())
    {
        auto sampledNormal = builder.makeTextureSample(
            TextureReference{ data.normalTexture },
            builder.makeCapabilityAccess(FragmentCapability::kVertexUV)
        );
        normal = builder.makeCall<TangentToWorldspace>({
            builder.makeMemberAccess(sampledNormal, "rgb")
        });
    }

    FragmentModule frag;
    frag.setParameter(FragmentModule::Parameter::eColor,          color);
    frag.setParameter(FragmentModule::Parameter::eNormal,         normal);
    frag.setParameter(FragmentModule::Parameter::eSpecularFactor, builder.makeConstant(data.specularCoefficient));
    frag.setParameter(FragmentModule::Parameter::eRoughness,      builder.makeConstant(data.roughness));
    frag.setParameter(FragmentModule::Parameter::eMetallicness,   builder.makeConstant(data.metallicness));
    frag.setParameter(FragmentModule::Parameter::eEmissive,
                      builder.makeExternalCall("float", {
                          builder.makeConstant(data.emissive)
                      }));

    const bool transparent = data.opacity < 1.0f;

    return MaterialData{ frag.build(std::move(builder), transparent), transparent };
}



SimpleMaterialRegistry::SimpleMaterialRegistry(AssetManager& assetManager)
    :
    assetManager(&assetManager)
{
}

auto SimpleMaterialRegistry::add(u_ptr<AssetSource<SimpleMaterial>> source) -> LocalID
{
    const auto matId = assetManager->create(makeMaterial(source->load()));
    LocalID id{ static_cast<ui32>(localIdPool.generate()) };
    baseMaterials.emplace(id, matId);

    return id;
}

void SimpleMaterialRegistry::remove(LocalID id)
{
    assetManager->destroy(baseMaterials[id]);
    baseMaterials.erase(id);
    localIdPool.free(static_cast<ui32>(id));
}

auto SimpleMaterialRegistry::getHandle(LocalID id) -> Handle
{
    return AssetHandle<SimpleMaterial>{ baseMaterials[id] };
}

} // namespace trc
