#include "trc/material/TorchMaterialSettings.h"

#include <cassert>

#include "trc/AssetDescriptor.h"
#include "trc/AssetPlugin.h"
#include "trc/GBuffer.h"
#include "trc/RasterPlugin.h"
#include "trc/ShaderLoader.h"
#include "trc/base/Logging.h"
#include "trc/material/FragmentShader.h"
#include "trc/util/TorchDirectories.h"



namespace trc
{

using shader::CapabilityConfig;

void addLightingRequirements(CapabilityConfig& config)
{
    using DescriptorBinding = CapabilityConfig::DescriptorBinding;

    auto shadowMatrixBufferResource = config.addResource(DescriptorBinding{
        .setName=RasterPlugin::SHADOW_DESCRIPTOR,
        .bindingIndex=0,
        .descriptorType="restrict readonly buffer",
        .descriptorName="ShadowMatrixBuffer",
        .descriptorContent="mat4 shadowMatrices[];",
    });
    auto shadowMapsResource = config.addResource(DescriptorBinding{
        .setName=RasterPlugin::SHADOW_DESCRIPTOR,
        .bindingIndex=1,
        .descriptorType="uniform sampler2D",
        .descriptorName="shadowMaps",
        .arrayCount=0,
    });
    config.addShaderExtension(shadowMapsResource, "GL_EXT_nonuniform_qualifier");
    config.linkCapability(
        FragmentCapability::kShadowMatrices,
        [=](shader::CapabilityBuildContext& ctx) {
            // Mark additional resources as used
            ctx.accessResource(shadowMapsResource);
            auto shadowMatBuffer = ctx.accessResource(shadowMatrixBufferResource);
            return ctx.builder.makeMemberAccess(shadowMatBuffer, "shadowMatrices");
        });

    auto lightBufferResource = config.addResource(DescriptorBinding{
        .setName=RasterPlugin::SCENE_DESCRIPTOR,
        .bindingIndex=0,
        .descriptorType="restrict readonly buffer",
        .descriptorName="LightBuffer",
        .descriptorContent=
            "uint numSunLights;"
            "uint numPointLights;"
            "uint numAmbientLights;"
            "Light lights[];"
    });
    config.addShaderInclude(lightBufferResource, util::Pathlet("material_utils/light.glsl"));
    config.linkCapability(FragmentCapability::kLightBuffer, lightBufferResource);
}

void addTextureSampleRequirements(CapabilityConfig& config)
{
    auto textureResource = config.addResource(CapabilityConfig::DescriptorBinding{
        .setName=AssetPlugin::ASSET_DESCRIPTOR,
        .bindingIndex=AssetDescriptor::getBindingIndex(AssetDescriptorBinding::eTextureSamplers),
        .descriptorType="uniform sampler2D",
        .descriptorName="textures",
        .arrayCount=0,
    });
    config.addShaderExtension(textureResource, "GL_EXT_nonuniform_qualifier");
    config.linkCapability(MaterialCapability::kTextureSample, textureResource);
}

auto makeFragmentCapabilityConfig() -> u_ptr<CapabilityConfig>
{
    using ShaderInput = CapabilityConfig::ShaderInput;
    using DescriptorBinding = CapabilityConfig::DescriptorBinding;

    // Helper that creates a capability builder for a simple member access on
    // a resource.
    auto makeResourceMemberAccess = [](auto resourceId, std::string member) {
        return [resourceId, member=std::move(member)](shader::CapabilityBuildContext& ctx) {
            auto resource = ctx.accessResource(resourceId);
            return ctx.builder.makeMemberAccess(resource, member);
        };
    };

    CapabilityConfig config;

    addTextureSampleRequirements(config);
    addLightingRequirements(config);

    auto fragListPointerImageResource = config.addResource(DescriptorBinding{
        .setName=RasterPlugin::G_BUFFER_DESCRIPTOR,
        .bindingIndex=GBufferDescriptor::getBindingIndex(GBufferDescriptorBinding::eTpFragHeadPointerImage),
        .descriptorType="uniform uimage2D",
        .descriptorName="fragmentListHeadPointer",
        .layoutQualifier="r32ui",
    });
    auto fragListAllocResource = config.addResource(DescriptorBinding{
        .setName=RasterPlugin::G_BUFFER_DESCRIPTOR,
        .bindingIndex=GBufferDescriptor::getBindingIndex(GBufferDescriptorBinding::eTpFragListEntryAllocator),
        .descriptorType="restrict buffer",
        .descriptorName="FragmentListAllocator",
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
        .descriptorContent="uvec4 fragmentList[];",
    });
    config.linkCapability(
        FragmentCapability::kNextFragmentListIndex,
        makeResourceMemberAccess(fragListAllocResource, "nextFragmentListIndex"));
    config.linkCapability(
        FragmentCapability::kMaxFragmentListIndex,
        makeResourceMemberAccess(fragListAllocResource, "maxFragmentListIndex"));
    config.linkCapability(FragmentCapability::kFragmentListHeadPointerImage,
                          fragListPointerImageResource);
    config.linkCapability(
        FragmentCapability::kFragmentListBuffer,
        makeResourceMemberAccess(fragListResource, "fragmentList"));

    auto cameraBufferResource = config.addResource(DescriptorBinding{
        .setName=RasterPlugin::GLOBAL_DATA_DESCRIPTOR,
        .bindingIndex=0,
        .descriptorType="uniform",
        .descriptorName="camera",
        .layoutQualifier="std140",
        .descriptorContent=
            "mat4 viewMatrix;\n"
            "mat4 projMatrix;\n"
            "mat4 inverseViewMatrix;\n"
            "mat4 inverseProjMatrix;\n"
    });
    config.linkCapability(
        MaterialCapability::kCameraWorldPos,
        [cameraBufferResource](shader::CapabilityBuildContext& ctx) {
            auto& b = ctx.builder;
            return b.makeMemberAccess(
                b.makeArrayAccess(
                    b.makeMemberAccess(ctx.accessResource(cameraBufferResource), "viewMatrix"),
                    b.makeConstant(2)),
                "xyz");
        });

    auto vWorldPos  = config.addResource(ShaderInput{ vec3{}, 0 });
    auto vUv        = config.addResource(ShaderInput{ vec2{}, 1 });
    auto vTbnMat    = config.addResource(ShaderInput{ mat3{}, 2 });

    config.linkCapability(MaterialCapability::kVertexWorldPos, vWorldPos);
    config.linkCapability(MaterialCapability::kVertexUV, vUv);
    config.linkCapability(MaterialCapability::kTangentToWorldSpaceMatrix, vTbnMat);
    config.linkCapability(
        MaterialCapability::kVertexNormal,
        [](shader::CapabilityBuildContext& ctx) {
            return ctx.builder.makeArrayAccess(
                ctx.builder.makeCapabilityAccess(MaterialCapability::kTangentToWorldSpaceMatrix),
                ctx.builder.makeConstant(2));
        }
    );

    return std::make_unique<CapabilityConfig>(std::move(config));
}

auto makeRayHitCapabilityConfig() -> u_ptr<CapabilityConfig>
{
    // ------------------------------------------------------------------------
    // Capabilities specific to the callable shader variant
    // ------------------------------------------------------------------------

    using Descriptor = CapabilityConfig::DescriptorBinding;
    using RayPayload = CapabilityConfig::RayPayload;
    namespace cap = RayHitCapability;

    CapabilityConfig config;

    // ------------------------------------------------------------------------
    // Global settings for ray tracing shader modules

    config.addGlobalShaderExtension("GL_EXT_ray_tracing");
    config.addGlobalShaderInclude(util::Pathlet("/ray_tracing/hit_utils.glsl"));

    // ------------------------------------------------------------------------
    // Define internal capabilities in `RayHitCapability`, i.e. access to
    // payload data.

    auto drawableDataBuf = config.addResource(Descriptor{
        .setName=AssetPlugin::ASSET_DESCRIPTOR,
        .bindingIndex=1,
        .descriptorType="restrict readonly buffer",
        .descriptorName="DrawableDataBuffer",
        .layoutQualifier="std430",
        .descriptorContent="DrawableData drawables[];"
    });
    config.linkCapability(RayHitCapability::kGeometryIndex,
        [=](shader::CapabilityBuildContext& ctx) {
            return ctx.builder.makeArrayAccess(
                ctx.builder.makeMemberAccess(ctx.accessResource(drawableDataBuf), "drawables"),
                ctx.builder.makeExternalIdentifier("gl_InstanceCustomIndexEXT")
            );
        });

    auto baryHitAttr = config.addResource(CapabilityConfig::HitAttribute{ vec2{} });
    config.linkCapability(RayHitCapability::kBarycentricCoords,
        [baryHitAttr](shader::CapabilityBuildContext& ctx) {
            auto& b = ctx.builder;
            auto bary = ctx.accessResource(baryHitAttr);
            auto baryX = b.makeMemberAccess(bary, "x");
            auto baryY = b.makeMemberAccess(bary, "y");
            return b.makeConstructor<vec3>(
                b.makeSub(b.makeConstant(1.0f), b.makeSub(baryX, baryY)),
                baryX,
                baryY
            );
        }
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
        .setName=AssetPlugin::ASSET_DESCRIPTOR,
        .bindingIndex=AssetDescriptor::getBindingIndex(AssetDescriptorBinding::eGeometryIndexBuffers),
        .descriptorType="restrict readonly buffer",
        .descriptorName="GeometryIndexBuffers",
        .arrayCount=0,
        .layoutQualifier="std430",
        .descriptorContent="uint indices[];"
    });
    auto vertexBufs = config.addResource(Descriptor{
        .setName=AssetPlugin::ASSET_DESCRIPTOR,
        .bindingIndex=AssetDescriptor::getBindingIndex(AssetDescriptorBinding::eGeometryVertexBuffers),
        .descriptorType="restrict readonly buffer",
        .descriptorName="GeometryVertexBuffers",
        .arrayCount=0,
        .layoutQualifier="std430",
        .descriptorContent="Vertex vertices[];"
    });
    config.addShaderInclude(indexBufs, util::Pathlet("/vertex.glsl"));

    config.linkCapability(MaterialCapability::kVertexUV,
        [=](shader::CapabilityBuildContext& ctx) {
            ctx.accessResource(indexBufs);
            ctx.accessResource(vertexBufs);
            return ctx.builder.makeCast<vec2>(ctx.builder.makeExternalCall("calcHitUv", {
                ctx.builder.makeCapabilityAccess(cap::kBarycentricCoords),
                ctx.builder.makeCapabilityAccess(cap::kGeometryIndex),
            }));
        }
    );
    config.linkCapability(MaterialCapability::kVertexNormal,
        [=](shader::CapabilityBuildContext& ctx) {
            ctx.accessResource(indexBufs);
            ctx.accessResource(vertexBufs);
            return ctx.builder.makeCast<vec3>(ctx.builder.makeExternalCall("calcHitNormal", {
                ctx.builder.makeCapabilityAccess(cap::kBarycentricCoords),
                ctx.builder.makeCapabilityAccess(cap::kGeometryIndex),
                ctx.builder.makeExternalIdentifier("gl_PrimitiveID"),
            }));
        }
    );

    // Implement world position capability
    config.linkCapability(MaterialCapability::kVertexWorldPos,
        [](shader::CapabilityBuildContext& ctx) {
            auto worldPosCalculation = ctx.builder.makeExternalCall("calcHitWorldPos", {});
            ctx.builder.annotateType(worldPosCalculation, vec3{});
            return worldPosCalculation;
        });

    // Implement tangentspace-to-worldspace matrix capability
    config.linkCapability(MaterialCapability::kTangentToWorldSpaceMatrix,
        [](shader::CapabilityBuildContext& ctx) {
            auto& b = ctx.builder;
            auto tangent = b.makeCast<vec3>(b.makeExternalCall("calcHitTangent", {
                b.makeCapabilityAccess(cap::kBarycentricCoords),
                b.makeCapabilityAccess(cap::kGeometryIndex),
                b.makeExternalIdentifier("gl_PrimitiveID"),
            }));
            return b.makeConstructor<mat3>(
                tangent,
                b.makeExternalCall("cross", {
                    tangent,
                    b.makeCapabilityAccess(MaterialCapability::kVertexNormal),
                }),
                b.makeCapabilityAccess(MaterialCapability::kVertexNormal)
            );
        });

    config.linkCapability(
        MaterialCapability::kCameraWorldPos,
        [](shader::CapabilityBuildContext& ctx) {
            return ctx.builder.makeExternalIdentifier("gl_WorldRayOriginEXT");
        });

    return std::make_unique<CapabilityConfig>(std::move(config));
}

auto makeProgramLinkerSettings() -> shader::ShaderProgramLinkSettings
{
    return shader::ShaderProgramLinkSettings{
        .preferredDescriptorSetIndices{
            { RasterPlugin::GLOBAL_DATA_DESCRIPTOR, 0 },
            { AssetPlugin::ASSET_DESCRIPTOR,        1 },
            { RasterPlugin::SCENE_DESCRIPTOR,       2 },
            { RasterPlugin::G_BUFFER_DESCRIPTOR,    3 },
            { RasterPlugin::SHADOW_DESCRIPTOR,      4 },
        },
        .inputLocationMapping{},
    };
}

auto makeShaderCompileOptions() -> u_ptr<shaderc::CompileOptions>
{
    auto opts = std::make_unique<shaderc::CompileOptions>(ShaderLoader::makeDefaultOptions());
    auto includer = std::make_unique<spirv::FileIncluder>(
        std::vector<fs::path>{
            util::getInternalShaderStorageDirectory(),
            util::getInternalShaderBinaryDirectory(),
        }
    );
    opts->SetIncluder(std::move(includer));

    return opts;
}



RuntimeTextureIndex::RuntimeTextureIndex(AssetReference<Texture> texture)
    :
    ShaderRuntimeConstant(ui32{}),
    texture(std::move(texture))
{
}

auto RuntimeTextureIndex::loadData() -> std::vector<std::byte>
{
    if (!texture.hasResolvedID())
    {
        log::error
            << "[In RuntimeTextureIndex::loadData]: Unable to load specialization constant data:"
            << " Referenced texture "
            << (texture.hasAssetPath() ? (texture.getAssetPath().string() + " ") : "")
            << "is not registered at the asset manager.";
        const ui32 defaultIndex{ 0 };
        return {
            reinterpret_cast<const std::byte*>(&defaultIndex),
            reinterpret_cast<const std::byte*>(&defaultIndex) + 4
        };
    }

    if (!runtimeHandle)
    {
        runtimeHandle = texture.getID().getDeviceDataHandle();
        assert(runtimeHandle);
    }
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

auto RuntimeTextureIndex::getTextureReference() -> AssetReference<Texture>
{
    return texture;
}

auto RuntimeTextureIndex::deserialize(const std::string& data) -> s_ptr<RuntimeTextureIndex>
{
    try {
        AssetReference<Texture> ref{ AssetPath{data} };
        return std::make_shared<RuntimeTextureIndex>(ref);
    }
    catch (const std::invalid_argument&) {
        // Happens if AssetPath constructor fails
        return nullptr;
    }
}

} // namespace trc
