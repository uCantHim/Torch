#include "trc/material/VertexShader.h"

#include <initializer_list>
#include <unordered_map>

#include "trc/AssetDescriptor.h"
#include "trc/AssetPlugin.h"
#include "trc/RasterPlugin.h"
#include "trc/material/FragmentShader.h"



namespace trc
{

using shader::FunctionType;
using shader::ShaderFunction;
using shader::ShaderOutputInterface;
namespace code = shader::code;

class GlPosition : public ShaderFunction
{
public:
    GlPosition()
        :
        ShaderFunction(
            "calcGlPosition",
            FunctionType{ { vec3{} }, vec4{} }
        )
    {}

    void build(shader::ShaderModuleBuilder& builder, const std::vector<code::Value>& args) override
    {
        auto viewproj = builder.makeMul(
            builder.makeCapabilityAccess(VertexCapability::kProjMatrix),
            builder.makeCapabilityAccess(VertexCapability::kViewMatrix)
        );
        builder.makeReturn(builder.makeMul(
            viewproj,
            builder.makeConstructor<vec4>(args[0], builder.makeConstant(1.0f))
        ));
    }
};

class ApplyAnimation : public ShaderFunction
{
public:
    ApplyAnimation()
        :
        ShaderFunction(
            "applyAnimationTransform",
            FunctionType{ { vec4{} }, vec4{} }
        )
    {}

    void build(shader::ShaderModuleBuilder& builder, const std::vector<code::Value>& args) override
    {
        auto value = args[0];

        auto anim = builder.makeCapabilityAccess(VertexCapability::kAnimIndex);
        auto keyframes = builder.makeCapabilityAccess(VertexCapability::kAnimKeyframes);
        auto weight = builder.makeCapabilityAccess(VertexCapability::kAnimFrameWeight);
        auto animRes = builder.makeExternalCall("applyAnimation", { anim, value, keyframes, weight });

        auto res = builder.makeConditional(
            builder.makeNotEqual(anim, builder.makeExternalIdentifier("NO_ANIMATION")),
            animRes,
            value);
        builder.makeReturn(res);
    }
};

class NormalToWorldspace : public ShaderFunction
{
public:
    NormalToWorldspace()
        :
        ShaderFunction("normalToWorldspace", FunctionType{ { vec4{} }, vec3{} })
    {}

    void build(shader::ShaderModuleBuilder& builder, const std::vector<code::Value>& args) override
    {
        auto model = builder.makeCapabilityAccess(VertexCapability::kModelMatrix);
        auto tiModel = builder.makeExternalCall(
            "transpose",
            { builder.makeExternalCall("inverse", {model}) }
        );
        auto normal = builder.makeMemberAccess(builder.makeMul(tiModel, args[0]), "xyz");
        builder.makeReturn(builder.makeExternalCall("normalize", { normal }));
    }
};



VertexModule::VertexModule(const VertexModuleCreateInfo& createInfo)
    :
    config(createInfo)
{
}

auto VertexModule::makeOutputs(shader::ShaderModuleBuilder& builder)
    -> shader::ShaderOutputInterface
{
    ShaderOutputInterface outputs;

    // Process gl_Position output
    if (auto pos = tryGetParameterValue(Out::vertexPosition))
    {
        // Cast to vec4
        auto val = builder.makeConstructor<vec4>(builder.makeCast<vec3>(*pos),
                                                 builder.makeConstant(1.0f));
        outputs.makeStore(builder.makeExternalIdentifier("gl_Position"), val);
    }
    else {
        outputs.makeStore(
            builder.makeExternalIdentifier("gl_Position"),
            builder.makeCall<GlPosition>(
                { builder.makeCapabilityAccess(MaterialCapability::kVertexWorldPos) }
            )
        );
    }

    return outputs;
}

auto VertexModule::makeCapabilityConfig(const VertexModuleCreateInfo& createInfo)
    -> u_ptr<shader::CapabilityConfig>
{
    return makeCapabilityConfigWithOutputs(createInfo);
}

auto VertexModule::makeInputCapabilityConfig() -> u_ptr<shader::CapabilityConfig>
{
    using shader::CapabilityConfig;

    static auto config = []{
        CapabilityConfig config;

        config.addGlobalShaderExtension("GL_GOOGLE_include_directive");

        auto cameraMatrices = config.addResource(CapabilityConfig::DescriptorBinding{
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

        auto modelPc = config.addResource(CapabilityConfig::PushConstant{
            DrawablePushConstIndex::eModelMatrix,
            mat4{},
        });
        auto animDataPc = config.addResource(CapabilityConfig::PushConstant{
            DrawablePushConstIndex::eAnimationData,
            code::makeExternalType("AnimationPushConstantData", 16),
        });
        config.addShaderInclude(animDataPc, util::Pathlet("material_utils/animation_data.glsl"));

        auto animMeta = config.addResource(CapabilityConfig::DescriptorBinding{
            .setName=AssetPlugin::ASSET_DESCRIPTOR,
            .bindingIndex=AssetDescriptor::getBindingIndex(AssetDescriptorBinding::eAnimationMetadata),
            .descriptorType="restrict readonly buffer",
            .descriptorName="AnimationMetaDataDescriptor",
            .layoutQualifier="std430",
            .descriptorContent="AnimationMetaData metas[];"
        });
        auto animBuffer = config.addResource(CapabilityConfig::DescriptorBinding{
            .setName=AssetPlugin::ASSET_DESCRIPTOR,
            .bindingIndex=AssetDescriptor::getBindingIndex(AssetDescriptorBinding::eAnimationData),
            .descriptorType="restrict readonly buffer",
            .descriptorName="AnimationDataDescriptor",
            .layoutQualifier="std140",
            .descriptorContent="mat4 boneMatrices[];"
        });
        config.addShaderInclude(animMeta, util::Pathlet("material_utils/animation_data.glsl"));
        config.linkCapability(VertexCapability::kAnimMetaBuffer, animMeta);
        config.linkCapability(VertexCapability::kAnimDataBuffer, animBuffer);

        auto vPos     = config.addResource(CapabilityConfig::ShaderInput{ vec3{}, 0 });
        auto vNormal  = config.addResource(CapabilityConfig::ShaderInput{ vec3{}, 1 });
        auto vUV      = config.addResource(CapabilityConfig::ShaderInput{ vec2{}, 2 });
        auto vTangent = config.addResource(CapabilityConfig::ShaderInput{ vec3{}, 3 });
        auto vBoneIndices = config.addResource(CapabilityConfig::ShaderInput{ uvec4{}, 4 });
        auto vBoneWeights = config.addResource(CapabilityConfig::ShaderInput{ vec4{}, 5 });

        config.linkCapability(VertexCapability::kPosition, vPos);
        config.linkCapability(VertexCapability::kNormal, vNormal);
        config.linkCapability(VertexCapability::kTangent, vTangent);
        config.linkCapability(VertexCapability::kUV, vUV);
        config.linkCapability(VertexCapability::kBoneIndices, vBoneIndices);
        config.linkCapability(VertexCapability::kBoneWeights, vBoneWeights);

        // Model matrix
        config.linkCapability(VertexCapability::kModelMatrix, modelPc);

        // Camera matrices
        auto makeResourceMemberAccess = [](auto resourceId, std::string member) {
            return [resourceId, member=std::move(member)](shader::CapabilityBuildContext& ctx) {
                auto resource = ctx.accessResource(resourceId);
                return ctx.builder.makeMemberAccess(resource, member);
            };
        };
        config.linkCapability(VertexCapability::kViewMatrix,
                              makeResourceMemberAccess(cameraMatrices, "viewMatrix"));
        config.linkCapability(VertexCapability::kProjMatrix,
                              makeResourceMemberAccess(cameraMatrices, "projMatrix"));

        // Animation data
        config.linkCapability(VertexCapability::kAnimIndex,
                              makeResourceMemberAccess(animDataPc, "animation"));
        config.linkCapability(VertexCapability::kAnimKeyframes,
                              makeResourceMemberAccess(animDataPc, "keyframes"));
        config.linkCapability(VertexCapability::kAnimFrameWeight,
                              makeResourceMemberAccess(animDataPc, "keyframeWeigth"));

        return config;
    }();

    return std::make_unique<CapabilityConfig>(config);
}

auto VertexModule::makeCapabilityConfigWithOutputs(const VertexModuleCreateInfo& createInfo)
    -> u_ptr<shader::CapabilityConfig>
{
    auto caps = makeInputCapabilityConfig();

    caps->linkCapability(
        MaterialCapability::kVertexWorldPos,
        [createInfo](shader::CapabilityBuildContext ctx) -> code::Value
        {
            auto& b = ctx.builder;

            auto objPos = b.makeCapabilityAccess(VertexCapability::kPosition);
            auto modelMat = b.makeCapabilityAccess(VertexCapability::kModelMatrix);
            auto objPos4 = b.makeConstructor<vec4>(objPos, b.makeConstant(1.0f));
            if (createInfo.animated)
            {
                b.includeCode("material_utils/animation.glsl", {
                    { "animationMetaDataDescriptorName", VertexCapability::kAnimMetaBuffer },
                    { "animationDataDescriptorName", VertexCapability::kAnimDataBuffer },
                    { "vertexBoneIndicesAttribName", VertexCapability::kBoneIndices },
                    { "vertexBoneWeightsAttribName", VertexCapability::kBoneWeights },
                });
                objPos4 = b.makeCall<ApplyAnimation>({ objPos4 });
            }

            auto worldPos = b.makeMul(modelMat, objPos4);
            return b.makeMemberAccess(worldPos, "xyz");
        }
    );

    const auto makeTbn = [createInfo](shader::CapabilityBuildContext& ctx) {
        auto& builder = ctx.builder;

        auto zero = builder.makeConstant(0.0f);
        auto normalObjspace = builder.makeCapabilityAccess(VertexCapability::kNormal);
        auto tangentObjspace = builder.makeCapabilityAccess(VertexCapability::kTangent);
        normalObjspace = builder.makeConstructor<vec4>(normalObjspace, zero);
        tangentObjspace = builder.makeConstructor<vec4>(tangentObjspace, zero);
        if (createInfo.animated) {
            normalObjspace = builder.makeCall<ApplyAnimation>({ normalObjspace });
            tangentObjspace = builder.makeCall<ApplyAnimation>({ tangentObjspace });
        }

        auto normal = builder.makeCall<NormalToWorldspace>({ normalObjspace });
        auto tangent = builder.makeCall<NormalToWorldspace>({ tangentObjspace });
        auto bitangent = builder.makeExternalCall("cross", { normal, tangent });

        auto tbn = builder.makeConstructor<mat3>(tangent, bitangent, normal);

        return tbn;
    };

    caps->linkCapability(MaterialCapability::kTangentToWorldSpaceMatrix, makeTbn);
    caps->linkCapability(
        MaterialCapability::kVertexUV,
        [](shader::CapabilityBuildContext& ctx) {
            return ctx.builder.makeCapabilityAccess(VertexCapability::kUV);
        }
    );

    return caps;
}

auto VertexModule::__makeVertexInputCapabilityConfig() -> u_ptr<shader::CapabilityConfig>
{
    using ShaderInput = shader::CapabilityConfig::ShaderInput;

    shader::CapabilityConfig config;

    auto vPos     = config.addResource(ShaderInput{ vec3{}, 0 });
    auto vNormal  = config.addResource(ShaderInput{ vec3{}, 1 });
    auto vUV      = config.addResource(ShaderInput{ vec2{}, 2 });
    auto vTangent = config.addResource(ShaderInput{ vec3{}, 3 });
    auto vBoneIndices = config.addResource(ShaderInput{ uvec4{}, 4 });
    auto vBoneWeights = config.addResource(ShaderInput{ vec4{}, 5 });

    config.linkCapability(VertexCapability::kPosition, vPos);
    config.linkCapability(VertexCapability::kNormal, vNormal);
    config.linkCapability(VertexCapability::kTangent, vTangent);
    config.linkCapability(VertexCapability::kUV, vUV);
    config.linkCapability(VertexCapability::kBoneIndices, vBoneIndices);
    config.linkCapability(VertexCapability::kBoneWeights, vBoneWeights);

    return std::make_unique<shader::CapabilityConfig>(std::move(config));
}

} // namespace trc
