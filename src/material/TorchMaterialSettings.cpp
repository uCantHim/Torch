#include "trc/material/TorchMaterialSettings.h"

#include "trc/AssetDescriptor.h"
#include "trc/GBuffer.h"
#include "trc/RasterPlugin.h"
#include "trc/base/Logging.h"
#include "trc/material/FragmentShader.h"



namespace trc
{

void addLightingRequirements(ShaderCapabilityConfig& config)
{
    using DescriptorBinding = ShaderCapabilityConfig::DescriptorBinding;
    auto& builder = config.getCodeBuilder();

    auto shadowMatrixBufferResource = config.addResource(DescriptorBinding{
        .setName=RasterPlugin::SHADOW_DESCRIPTOR,
        .bindingIndex=0,
        .descriptorType="restrict readonly buffer",
        .descriptorName="ShadowMatrixBuffer",
        .isArray=false,
        .arrayCount=0,
        .layoutQualifier=std::nullopt,
        .descriptorContent="mat4 shadowMatrices[];",
    });
    auto shadowMapsResource = config.addResource(DescriptorBinding{
        .setName=RasterPlugin::SHADOW_DESCRIPTOR,
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
        builder.makeMemberAccess(config.accessResource(shadowMatrixBufferResource), "shadowMatrices"),
        { shadowMapsResource, shadowMatrixBufferResource }
    );

    auto lightBufferResource = config.addResource(DescriptorBinding{
        .setName=RasterPlugin::SCENE_DESCRIPTOR,
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
}

void addTextureSampleRequirements(ShaderCapabilityConfig& config)
{
    auto textureResource = config.addResource(ShaderCapabilityConfig::DescriptorBinding{
        .setName=RasterPlugin::ASSET_DESCRIPTOR,
        .bindingIndex=AssetDescriptor::getBindingIndex(AssetDescriptorBinding::eTextureSamplers),
        .descriptorType="uniform sampler2D",
        .descriptorName="textures",
        .isArray=true,
        .arrayCount=0,
        .layoutQualifier=std::nullopt,
        .descriptorContent=std::nullopt,
    });
    config.addShaderExtension(textureResource, "GL_EXT_nonuniform_qualifier");
    config.linkCapability(MaterialCapability::kTextureSample, textureResource);
}

auto makeFragmentCapabilityConfig() -> ShaderCapabilityConfig
{
    using ShaderInput = ShaderCapabilityConfig::ShaderInput;
    using DescriptorBinding = ShaderCapabilityConfig::DescriptorBinding;

    ShaderCapabilityConfig config;
    auto& code = config.getCodeBuilder();

    addTextureSampleRequirements(config);
    addLightingRequirements(config);

    auto fragListPointerImageResource = config.addResource(DescriptorBinding{
        .setName=RasterPlugin::G_BUFFER_DESCRIPTOR,
        .bindingIndex=GBufferDescriptor::getBindingIndex(GBufferDescriptorBinding::eTpFragHeadPointerImage),
        .descriptorType="uniform uimage2D",
        .descriptorName="fragmentListHeadPointer",
        .isArray=false,
        .arrayCount=0,
        .layoutQualifier="r32ui",
        .descriptorContent=std::nullopt,
    });
    auto fragListAllocResource = config.addResource(DescriptorBinding{
        .setName=RasterPlugin::G_BUFFER_DESCRIPTOR,
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
    auto fragListResource = config.addResource(DescriptorBinding{
        .setName=RasterPlugin::G_BUFFER_DESCRIPTOR,
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

    auto cameraBufferResource = config.addResource(DescriptorBinding{
        .setName=RasterPlugin::GLOBAL_DATA_DESCRIPTOR,
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
        MaterialCapability::kCameraWorldPos,
        code.makeMemberAccess(
            code.makeArrayAccess(
                code.makeMemberAccess(config.accessResource(cameraBufferResource), "viewMatrix"),
                code.makeConstant(2)
            ),
            "xyz"
        ),
        { cameraBufferResource }
    );

    auto vWorldPos  = config.addResource(ShaderInput{ vec3{}, 0 });
    auto vUv        = config.addResource(ShaderInput{ vec2{}, 1 });
    auto vTbnMat    = config.addResource(ShaderInput{ mat3{}, 3 });

    config.linkCapability(MaterialCapability::kVertexWorldPos, vWorldPos);
    config.linkCapability(MaterialCapability::kVertexUV, vUv);
    config.linkCapability(MaterialCapability::kTangentToWorldSpaceMatrix, vTbnMat);
    config.linkCapability(
        MaterialCapability::kVertexNormal,
        code.makeArrayAccess(config.accessResource(vTbnMat), code.makeConstant(2)),
        { vTbnMat }
    );

    return config;
}

auto makeRayHitCapabilityConfig() -> ShaderCapabilityConfig
{
    // ------------------------------------------------------------------------
    // Capabilities specific to the callable shader variant
    // ------------------------------------------------------------------------

    using Descriptor = ShaderCapabilityConfig::DescriptorBinding;
    using RayPayload = ShaderCapabilityConfig::RayPayload;

    namespace cap = RayHitCapability;

    ShaderCapabilityConfig config;
    auto& builder = config.getCodeBuilder();

    // ------------------------------------------------------------------------
    // Global settings for ray tracing shader modules

    config.addGlobalShaderExtension("GL_EXT_ray_tracing");
    config.addGlobalShaderInclude(util::Pathlet("/ray_tracing/hit_utils.glsl"));

    // ------------------------------------------------------------------------
    // Define internal capabilities in `RayHitCapability`, i.e. access to
    // payload data.

    auto drawableDataBuf = config.addResource(Descriptor{
        .setName=RasterPlugin::ASSET_DESCRIPTOR,
        .bindingIndex=1,
        .descriptorType="restrict readonly buffer",
        .descriptorName="DrawableDataBuffer",
        .isArray=false,
        .layoutQualifier="std430",
        .descriptorContent="DrawableData drawables[];"
    });
    config.linkCapability(RayHitCapability::kGeometryIndex,
        builder.makeArrayAccess(
            builder.makeMemberAccess(config.accessResource(drawableDataBuf), "drawables"),
            builder.makeExternalIdentifier("gl_InstanceCustomIndexEXT")
        ),
        { drawableDataBuf }
    );

    auto baryHitAttr = config.addResource(ShaderCapabilityConfig::HitAttribute{ vec2{} });
    auto baryX = builder.makeMemberAccess(config.accessResource(baryHitAttr), "x");
    auto baryY = builder.makeMemberAccess(config.accessResource(baryHitAttr), "y");
    config.linkCapability(RayHitCapability::kBarycentricCoords,
        builder.makeConstructor<vec3>(
            builder.makeSub(builder.makeConstant(1.0f), builder.makeSub(baryX, baryY)),
            baryX,
            baryY
        ),
        { baryHitAttr }
    );

    auto outputPayload = config.addResource(RayPayload{ .type=vec3{}, .incoming=true });
    config.linkCapability(RayHitCapability::kOutColor, outputPayload);

    // ------------------------------------------------------------------------
    // Define user-exposed capabilities in `MaterialCapability`. These are
    // also implemented by the fragment shader capability configuration and
    // should implement the same semantics here.

    addTextureSampleRequirements(config);
    addLightingRequirements(config);

    auto indexBufs = config.addResource(Descriptor{
        .setName=RasterPlugin::ASSET_DESCRIPTOR,
        .bindingIndex=AssetDescriptor::getBindingIndex(AssetDescriptorBinding::eGeometryIndexBuffers),
        .descriptorType="restrict readonly buffer",
        .descriptorName="GeometryIndexBuffers",
        .isArray=true,
        .arrayCount=0,
        .layoutQualifier="std430",
        .descriptorContent="uint indices[];"
    });
    auto vertexBufs = config.addResource(Descriptor{
        .setName=RasterPlugin::ASSET_DESCRIPTOR,
        .bindingIndex=AssetDescriptor::getBindingIndex(AssetDescriptorBinding::eGeometryVertexBuffers),
        .descriptorType="restrict readonly buffer",
        .descriptorName="GeometryVertexBuffers",
        .isArray=true,
        .arrayCount=0,
        .layoutQualifier="std430",
        .descriptorContent="Vertex vertices[];"
    });
    config.addShaderInclude(indexBufs, util::Pathlet("/vertex.glsl"));

    config.linkCapability(MaterialCapability::kVertexUV,
        builder.makeCast<vec2>(builder.makeExternalCall("calcHitUv", {
            config.accessCapability(cap::kBarycentricCoords),
            config.accessCapability(cap::kGeometryIndex),
        })),
        { indexBufs, vertexBufs }
    );
    config.linkCapability(MaterialCapability::kVertexNormal,
        builder.makeCast<vec3>(builder.makeExternalCall("calcHitNormal", {
            config.accessCapability(cap::kBarycentricCoords),
            config.accessCapability(cap::kGeometryIndex),
            builder.makeExternalIdentifier("gl_PrimitiveID"),
        })),
        { indexBufs, vertexBufs }
    );

    // Implement world position capability
    auto worldPosCalculation = builder.makeExternalCall("calcHitWorldPos", {});
    builder.annotateType(worldPosCalculation, vec3{});
    config.linkCapability(MaterialCapability::kVertexWorldPos, worldPosCalculation, {});

    // Implement tangentspace-to-worldspace matrix capability
    auto tangent = builder.makeCast<vec3>(builder.makeExternalCall("calcHitTangent", {
        config.accessCapability(cap::kBarycentricCoords),
        config.accessCapability(cap::kGeometryIndex),
        builder.makeExternalIdentifier("gl_PrimitiveID"),
    }));
    config.linkCapability(MaterialCapability::kTangentToWorldSpaceMatrix,
        builder.makeConstructor<mat3>(
            tangent,
            builder.makeExternalCall("cross", {
                tangent,
                config.accessCapability(MaterialCapability::kVertexNormal),
            }),
            config.accessCapability(MaterialCapability::kVertexNormal)
        ),
        {} // No additional resources
    );

    config.linkCapability(
        MaterialCapability::kCameraWorldPos,
        builder.makeExternalIdentifier("gl_WorldRayOriginEXT"),
        {}
    );

    return config;
}

auto makeShaderDescriptorConfig() -> ShaderDescriptorConfig
{
    return ShaderDescriptorConfig{
        .descriptorInfos{
            { RasterPlugin::GLOBAL_DATA_DESCRIPTOR, { 0, true } },
            { RasterPlugin::ASSET_DESCRIPTOR,       { 1, true } },
            { RasterPlugin::SCENE_DESCRIPTOR,       { 2, true } },
            { RasterPlugin::G_BUFFER_DESCRIPTOR,    { 3, true } },
            { RasterPlugin::SHADOW_DESCRIPTOR,      { 4, true } },
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
