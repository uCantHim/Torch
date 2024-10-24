#include "trc/assets/SimpleMaterial.h"

#include "trc/assets/AssetSource.h"
#include "trc/AssetPlugin.h"
#include "trc/assets/import/InternalFormat.h"
#include "trc/base/Logging.h"
#include "trc/material/CommonShaderFunctions.h"
#include "trc/material/FragmentShader.h"
#include "trc/material/TorchMaterialSettings.h"
#include "trc/material/VertexShader.h"
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
    constexpr ui32 kMatBufferIndexPcId = DrawablePushConstIndex::eMaterialData;
    constexpr auto kCapCurrentMat = "simplemat_param_struct";

    ShaderModuleBuilder builder;
    auto capabilities = makeFragmentCapabilityConfig();

    // Declare resources: the buffer index (runtime value) and the buffer descriptor
    auto matIndex = capabilities.addResource(ShaderCapabilityConfig::PushConstant{
        ui32{},
        kMatBufferIndexPcId
    });
    auto desc = capabilities.addResource(ShaderCapabilityConfig::DescriptorBinding{
        .setName=AssetPlugin::ASSET_DESCRIPTOR,
        .bindingIndex=AssetDescriptor::getBindingIndex(AssetDescriptorBinding::eMaterialParameterBuffer),
        .descriptorType="restrict readonly buffer",
        .descriptorName="MaterialParameterBuffer",
        .isArray=false,
        .layoutQualifier="std430",
        .descriptorContent="MaterialParameters materials[];",
    });
    capabilities.addShaderInclude(desc, util::Pathlet{"material_utils/simple_material.glsl"});

    // Create the current-material-struct capability
    capabilities.linkCapability(
        kCapCurrentMat,
        builder.makeArrayAccess(
            builder.makeMemberAccess(capabilities.accessResource(desc), "materials"),
            capabilities.accessResource(matIndex)
        ),
        { matIndex, desc }
    );

    auto mat = builder.makeCapabilityAccess(kCapCurrentMat);
    auto colorParam        = builder.makeMemberAccess(mat, "color");
    auto specularParam     = builder.makeMemberAccess(mat, "specularFactor");
    auto roughnessParam    = builder.makeMemberAccess(mat, "roughness");
    auto metallicnessParam = builder.makeMemberAccess(mat, "metallicness");
    auto emissiveParam     = builder.makeMemberAccess(mat, "emissive");

    // TODO: This remains the old, non-dynamic way until the buffer is implemented.
    colorParam        = builder.makeConstant(data.color);
    specularParam     = builder.makeConstant(data.specularCoefficient);
    roughnessParam    = builder.makeConstant(data.roughness);
    metallicnessParam = builder.makeConstant(data.metallicness);
    emissiveParam     = builder.makeConstant(data.emissive);

    // Build output values
    code::Value opacity = builder.makeConstant(data.opacity);
    code::Value color = builder.makeConstructor<vec4>(colorParam, opacity);
    if (!data.albedoTexture.empty())
    {
        color = builder.makeCall<TextureSample>({
            builder.makeSpecializationConstant(
                std::make_shared<RuntimeTextureIndex>(data.albedoTexture)
            ),
            builder.makeCapabilityAccess(MaterialCapability::kVertexUV)
        });
    }

    code::Value normal = builder.makeCapabilityAccess(MaterialCapability::kVertexNormal);;
    if (!data.normalTexture.empty())
    {
        auto sampledNormal = builder.makeCall<TextureSample>({
            builder.makeSpecializationConstant(
                std::make_shared<RuntimeTextureIndex>(data.normalTexture)
            ),
            builder.makeCapabilityAccess(MaterialCapability::kVertexUV)
        });
        normal = builder.makeCall<TangentToWorldspace>({
            builder.makeMemberAccess(sampledNormal, "rgb")
        });
    }

    FragmentModule frag;
    frag.setParameter(FragmentModule::Parameter::eColor,          color);
    frag.setParameter(FragmentModule::Parameter::eNormal,         normal);
    frag.setParameter(FragmentModule::Parameter::eSpecularFactor, specularParam);
    frag.setParameter(FragmentModule::Parameter::eRoughness,      roughnessParam);
    frag.setParameter(FragmentModule::Parameter::eMetallicness,   metallicnessParam);
    frag.setParameter(FragmentModule::Parameter::eEmissive,
                      builder.makeCast<float>(emissiveParam));

    const bool transparent = data.opacity < 1.0f;
    return MaterialData{
        frag.build(std::move(builder), transparent, capabilities),
        transparent
    };
}

} // namespace trc
