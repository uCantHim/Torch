#include "trc/material/TorchMaterialSettings.h"

#include "trc/AssetDescriptor.h"
#include "trc/GBuffer.h"
#include "trc/TorchRenderConfig.h"
#include "trc/base/Logging.h"
#include "trc/material/FragmentShader.h"



namespace trc
{

auto makeFragmentCapabiltyConfig() -> ShaderCapabilityConfig
{
    ShaderCapabilityConfig config;
    auto& code = config.getCodeBuilder();

    auto textureResource = config.addResource(ShaderCapabilityConfig::DescriptorBinding{
        .setName=TorchRenderConfig::ASSET_DESCRIPTOR,
        .bindingIndex=AssetDescriptor::getBindingIndex(AssetDescriptorBinding::eTextureSamplers),
        .descriptorType="uniform sampler2D",
        .descriptorName="textures",
        .isArray=true,
        .arrayCount=0,
        .layoutQualifier=std::nullopt,
        .descriptorContent=std::nullopt,
    });
    config.addShaderExtension(textureResource, "GL_EXT_nonuniform_qualifier");
    config.linkCapability(FragmentCapability::kTextureSample, textureResource);

    auto fragListPointerImageResource = config.addResource(ShaderCapabilityConfig::DescriptorBinding{
        .setName=TorchRenderConfig::G_BUFFER_DESCRIPTOR,
        .bindingIndex=GBufferDescriptor::getBindingIndex(GBufferDescriptorBinding::eTpFragHeadPointerImage),
        .descriptorType="uniform uimage2D",
        .descriptorName="fragmentListHeadPointer",
        .isArray=false,
        .arrayCount=0,
        .layoutQualifier="r32ui",
        .descriptorContent=std::nullopt,
    });
    auto fragListAllocResource = config.addResource(ShaderCapabilityConfig::DescriptorBinding{
        .setName=TorchRenderConfig::G_BUFFER_DESCRIPTOR,
        .bindingIndex=GBufferDescriptor::getBindingIndex(GBufferDescriptorBinding::eTpFragListEntryAllocator),
        .descriptorType="restrict buffer",
        .descriptorName="FragmentListAllocator",
        .isArray=false,
        .arrayCount=0,
        .layoutQualifier=std::nullopt,
        .descriptorContent=
            "uint nextFragmentListIndex;\n"
            "uint maxFragmentListIndex;"
    });
    auto fragListResource = config.addResource(ShaderCapabilityConfig::DescriptorBinding{
        .setName=TorchRenderConfig::G_BUFFER_DESCRIPTOR,
        .bindingIndex=GBufferDescriptor::getBindingIndex(GBufferDescriptorBinding::eTpFragListBuffer),
        .descriptorType="restrict buffer",
        .descriptorName="FragmentListBuffer",
        .isArray=false,
        .arrayCount=0,
        .layoutQualifier=std::nullopt,
        .descriptorContent="uvec4 fragmentList[];",
    });
    config.linkCapability(
        FragmentCapability::kNextFragmentListIndex,
        code.makeMemberAccess(config.accessResource(fragListAllocResource), "nextFragmentListIndex"),
        { fragListAllocResource }
    );
    config.linkCapability(
        FragmentCapability::kMaxFragmentListIndex,
        code.makeMemberAccess(config.accessResource(fragListAllocResource), "maxFragmentListIndex"),
        { fragListAllocResource }
    );
    config.linkCapability(FragmentCapability::kFragmentListHeadPointerImage,
                          fragListPointerImageResource);
    config.linkCapability(
        FragmentCapability::kFragmentListBuffer,
        code.makeMemberAccess(config.accessResource(fragListResource), "fragmentList"),
        { fragListResource }
    );

    auto shadowMatrixBufferResource = config.addResource(ShaderCapabilityConfig::DescriptorBinding{
        .setName=TorchRenderConfig::SHADOW_DESCRIPTOR,
        .bindingIndex=0,
        .descriptorType="restrict readonly buffer",
        .descriptorName="ShadowMatrixBuffer",
        .isArray=false,
        .arrayCount=0,
        .layoutQualifier=std::nullopt,
        .descriptorContent="mat4 shadowMatrices[];",
    });
    auto shadowMapsResource = config.addResource(ShaderCapabilityConfig::DescriptorBinding{
        .setName=TorchRenderConfig::SHADOW_DESCRIPTOR,
        .bindingIndex=1,
        .descriptorType="uniform sampler2D",
        .descriptorName="shadowMaps",
        .isArray=true,
        .arrayCount=0,
        .layoutQualifier=std::nullopt,
        .descriptorContent=std::nullopt,
    });
    config.addShaderExtension(shadowMapsResource, "GL_EXT_nonuniform_qualifier");
    config.linkCapability(
        FragmentCapability::kShadowMatrices,
        code.makeMemberAccess(config.accessResource(shadowMatrixBufferResource), "shadowMatrices"),
        { shadowMapsResource, shadowMatrixBufferResource }
    );

    auto lightBufferResource = config.addResource(ShaderCapabilityConfig::DescriptorBinding{
        .setName=TorchRenderConfig::SCENE_DESCRIPTOR,
        .bindingIndex=0,
        .descriptorType="restrict readonly buffer",
        .descriptorName="LightBuffer",
        .isArray=false,
        .arrayCount=0,
        .layoutQualifier=std::nullopt,
        .descriptorContent=
            "uint numSunLights;"
            "uint numPointLights;"
            "uint numAmbientLights;"
            "Light lights[];"
    });
    config.addShaderInclude(lightBufferResource, util::Pathlet("material_utils/light.glsl"));
    config.linkCapability(FragmentCapability::kLightBuffer, lightBufferResource);

    auto cameraBufferResource = config.addResource(ShaderCapabilityConfig::DescriptorBinding{
        .setName=TorchRenderConfig::GLOBAL_DATA_DESCRIPTOR,
        .bindingIndex=0,
        .descriptorType="uniform",
        .descriptorName="camera",
        .isArray=false,
        .arrayCount=0,
        .layoutQualifier="std140",
        .descriptorContent=
            "mat4 viewMatrix;\n"
            "mat4 projMatrix;\n"
            "mat4 inverseViewMatrix;\n"
            "mat4 inverseProjMatrix;\n"
    });
    config.linkCapability(
        FragmentCapability::kCameraWorldPos,
        code.makeMemberAccess(
            code.makeArrayAccess(
                code.makeMemberAccess(config.accessResource(cameraBufferResource), "viewMatrix"),
                code.makeConstant(2)
            ),
            "xyz"
        ),
        { cameraBufferResource }
    );

    auto vWorldPos  = config.addResource(ShaderCapabilityConfig::ShaderInput{ vec3{}, 0 });
    auto vUv        = config.addResource(ShaderCapabilityConfig::ShaderInput{ vec2{}, 1 });
    auto vTbnMat    = config.addResource(ShaderCapabilityConfig::ShaderInput{ mat3{}, 3 });

    config.linkCapability(FragmentCapability::kVertexWorldPos, vWorldPos);
    config.linkCapability(FragmentCapability::kVertexUV, vUv);
    config.linkCapability(FragmentCapability::kTangentToWorldSpaceMatrix, vTbnMat);
    config.linkCapability(
        FragmentCapability::kVertexNormal,
        code.makeArrayAccess(config.accessResource(vTbnMat), code.makeConstant(2)),
        { vTbnMat }
    );

    return config;
}

auto makeShaderDescriptorConfig() -> ShaderDescriptorConfig
{
    return ShaderDescriptorConfig{
        .descriptorInfos{
            { TorchRenderConfig::GLOBAL_DATA_DESCRIPTOR, { 0, true } },
            { TorchRenderConfig::ASSET_DESCRIPTOR,       { 1, true } },
            { TorchRenderConfig::SCENE_DESCRIPTOR,       { 2, true } },
            { TorchRenderConfig::G_BUFFER_DESCRIPTOR,    { 3, true } },
            { TorchRenderConfig::SHADOW_DESCRIPTOR,      { 4, true } },
        }
    };
}



RuntimeTextureIndex::RuntimeTextureIndex(AssetReference<Texture> texture)
    :
    ShaderRuntimeConstant(ui32{}),
    texture(std::move(texture))
{
}

auto RuntimeTextureIndex::loadData() -> std::vector<std::byte>
{
    assert(texture.hasResolvedID());

    runtimeHandle = texture.getID().getDeviceDataHandle();
    const ui32 index = runtimeHandle->getDeviceIndex();
    return {
        reinterpret_cast<const std::byte*>(&index),
        reinterpret_cast<const std::byte*>(&index) + 4
    };
}

auto RuntimeTextureIndex::serialize() const -> std::string
{
    if (!texture.hasAssetPath())
    {
        log::warn << log::here()
                  << ": Tried to serialize a texture reference without an asset path."
                     " This will cause a fatal error during de-serialization.";
        return "";
    }

    return texture.getAssetPath().string();
}

} // namespace trc
