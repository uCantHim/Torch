#include "trc/material/VertexShader.h"

#include <initializer_list>
#include <unordered_map>

#include "trc/material/FragmentShader.h"



namespace trc
{

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

    void build(ShaderModuleBuilder& builder, std::vector<code::Value> args) override
    {
        auto viewproj = builder.makeMul(
            builder.makeCapabilityAccess(VertexCapability::kProjMatrix),
            builder.makeCapabilityAccess(VertexCapability::kViewMatrix)
        );
        builder.makeReturn(builder.makeMul(
            viewproj,
            builder.makeExternalCall("vec4", { args[0], builder.makeConstant(1.0f) })
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

    void build(ShaderModuleBuilder& builder, std::vector<code::Value> args) override
    {
        auto anim = builder.makeCapabilityAccess(VertexCapability::kAnimIndex);
        auto keyframes = builder.makeCapabilityAccess(VertexCapability::kAnimKeyframes);
        auto weight = builder.makeCapabilityAccess(VertexCapability::kAnimFrameWeight);
        auto res = builder.makeExternalCall("applyAnimation", { anim, args[0], keyframes, weight });
        builder.makeReturn(res);
    }
};

class ToVec4 : public ShaderFunction
{
public:
    ToVec4()
        :
        ShaderFunction("toVec4", FunctionType{ { vec3{}, float{} }, vec4{} })
    {}

    void build(ShaderModuleBuilder& builder, std::vector<code::Value> args) override
    {
        builder.makeReturn(builder.makeExternalCall("vec4", args));
    }
};

class NormalToWorldspace : public ShaderFunction
{
public:
    NormalToWorldspace()
        :
        ShaderFunction("normalToWorldspace", FunctionType{ { vec4{} }, vec3{} })
    {}

    void build(ShaderModuleBuilder& builder, std::vector<code::Value> args) override
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



VertexShaderBuilder::VertexShaderBuilder(
    ShaderModule fragmentResult,
    bool animated)
    :
    fragment(std::move(fragmentResult)),
    configs(makeVertexCapabilityConfig()),
    builder(std::move(configs.first))
{
    fragCapabilityProviders = {
        {
            FragmentCapability::kVertexWorldPos,
            [this, animated]() -> code::Value
            {
                auto objPos = builder.makeCapabilityAccess(VertexCapability::kPosition);
                auto modelMat = builder.makeCapabilityAccess(VertexCapability::kModelMatrix);
                auto objPos4 = builder.makeCall<ToVec4>({ objPos, builder.makeConstant(1.0f) });
                if (animated)
                {
                    builder.includeCode(util::Pathlet("material_utils/animation.glsl"), {
                        { "animationMetaDataDescriptorName", VertexCapability::kAnimMetaBuffer },
                        { "animationDataDescriptorName", VertexCapability::kAnimDataBuffer },
                        { "vertexBoneIndicesAttribName", VertexCapability::kBoneIndices },
                        { "vertexBoneWeightsAttribName", VertexCapability::kBoneWeights },
                    });
                    objPos4 = builder.makeCall<ApplyAnimation>({ objPos4 });
                }

                auto worldPos = builder.makeMul(modelMat, objPos4);
                return builder.makeMemberAccess(worldPos, "xyz");
            }()
        },
        {
            FragmentCapability::kTangentToWorldSpaceMatrix,
            [this, animated]() -> code::Value {
                auto zero = builder.makeConstant(0.0f);

                auto normalObjspace = builder.makeCapabilityAccess(VertexCapability::kNormal);
                auto tangentObjspace = builder.makeCapabilityAccess(VertexCapability::kTangent);
                normalObjspace = builder.makeCall<ToVec4>({ normalObjspace, zero });
                tangentObjspace = builder.makeCall<ToVec4>({ tangentObjspace, zero });
                if (animated) {
                    normalObjspace = builder.makeCall<ApplyAnimation>({ normalObjspace });
                    tangentObjspace = builder.makeCall<ApplyAnimation>({ tangentObjspace });
                }

                auto normal = builder.makeCall<NormalToWorldspace>({ normalObjspace });
                auto tangent = builder.makeCall<NormalToWorldspace>({ tangentObjspace });
                auto bitangent = builder.makeExternalCall("cross", { normal, tangent });

                auto tbn = builder.makeExternalCall("mat3", { tangent, bitangent, normal });

                return tbn;
            }()
        },
        { FragmentCapability::kVertexUV, builder.makeCapabilityAccess(VertexCapability::kUV) }
    };
}

auto VertexShaderBuilder::buildVertexShader() -> std::pair<ShaderModule, MaterialRuntimeConfig>
{
    ShaderOutputNode vertNode;
    for (const auto& out : fragment.getRequiredShaderInputs())
    {
        auto output = vertNode.addOutput(out.location, out.type);
        auto param = vertNode.addParameter(out.type);
        vertNode.linkOutput(param, output, "");

        try {
            auto inputNode = fragCapabilityProviders.at(out.capability);
            vertNode.setParameter(param, inputNode);
        }
        catch (const std::out_of_range&)
        {
            if constexpr (enableVerboseLogging)
            {
                std::cout << "Warning: [In VertexShaderBuilder::buildVertexShader]: Fragment"
                          << " capability \"" << out.capability.getString()
                          << "\" is not implemented.\n";
            }
        }
    }

    // Always add the gl_Position output
    builder.makeAssignment(
        builder.makeExternalIdentifier("gl_Position"),
        builder.makeCall<GlPosition>(
            { fragCapabilityProviders.at(FragmentCapability::kVertexWorldPos) }
        )
    );

    ShaderModuleCompiler vertCompiler;
    return { vertCompiler.compile(vertNode, builder), configs.second };
}

auto VertexShaderBuilder::makeVertexCapabilityConfig()
    -> std::pair<ShaderCapabilityConfig, MaterialRuntimeConfig>
{
    ShaderCapabilityConfig config;
    auto& code = config.getCodeBuilder();

    config.addGlobalShaderExtension("GL_GOOGLE_include_directive");

    auto cameraMatrices = config.addResource(ShaderCapabilityConfig::DescriptorBinding{
        .setName="global_data",
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

    auto modelPc = config.addResource(ShaderCapabilityConfig::PushConstant{ mat4{} });
    auto matIndexPc = config.addResource(ShaderCapabilityConfig::PushConstant{ uint{} });
    auto animDataPc = config.addResource(ShaderCapabilityConfig::PushConstant{
        sizeof(AnimationDeviceData), "AnimationPushConstantData"
    });
    config.addShaderInclude(animDataPc, util::Pathlet("material_utils/animation_data.glsl"));

    auto animMeta = config.addResource(ShaderCapabilityConfig::DescriptorBinding{
        .setName="asset_registry",
        .bindingIndex=4,
        .descriptorType="restrict readonly buffer",
        .descriptorName="AnimationMetaDataDescriptor",
        .isArray=false,
        .arrayCount=0,
        .layoutQualifier="std430",
        .descriptorContent="AnimationMetaData metas[];"
    });
    auto animBuffer = config.addResource(ShaderCapabilityConfig::DescriptorBinding{
        .setName="asset_registry",
        .bindingIndex=5,
        .descriptorType="restrict readonly buffer",
        .descriptorName="AnimationDataDescriptor",
        .isArray=false,
        .arrayCount=0,
        .layoutQualifier="std140",
        .descriptorContent="mat4 boneMatrices[];"
    });
    config.addShaderInclude(animMeta, util::Pathlet("material_utils/animation_data.glsl"));
    config.linkCapability(VertexCapability::kAnimMetaBuffer, animMeta, bool{});
    config.linkCapability(VertexCapability::kAnimDataBuffer, animBuffer, bool{});

    auto vPos     = config.addResource(ShaderCapabilityConfig::ShaderInput{ vec3{}, 0 });
    auto vNormal  = config.addResource(ShaderCapabilityConfig::ShaderInput{ vec3{}, 1 });
    auto vUV      = config.addResource(ShaderCapabilityConfig::ShaderInput{ vec2{}, 2 });
    auto vTangent = config.addResource(ShaderCapabilityConfig::ShaderInput{ vec3{}, 3 });
    auto vBoneIndices = config.addResource(ShaderCapabilityConfig::ShaderInput{ uvec4{}, 4 });
    auto vBoneWeights = config.addResource(ShaderCapabilityConfig::ShaderInput{ vec4{}, 5 });

    config.linkCapability(VertexCapability::kPosition, vPos, vec3{});
    config.linkCapability(VertexCapability::kNormal, vNormal, vec3{});
    config.linkCapability(VertexCapability::kTangent, vTangent, vec3{});
    config.linkCapability(VertexCapability::kUV, vUV, vec2{});
    config.linkCapability(VertexCapability::kBoneIndices, vBoneIndices, uvec4{});
    config.linkCapability(VertexCapability::kBoneWeights, vBoneWeights, vec4{});

    // Model matrix
    config.linkCapability(VertexCapability::kModelMatrix, modelPc, mat4{});

    // Camera matrices
    auto camera = config.accessResource(cameraMatrices);
    config.linkCapability(VertexCapability::kViewMatrix,
                          code.makeMemberAccess(camera, "viewMatrix"), mat4{},
                          { cameraMatrices });
    config.linkCapability(VertexCapability::kProjMatrix,
                          code.makeMemberAccess(camera, "projMatrix"), mat4{},
                          { cameraMatrices });

    // Animation data
    auto animData = config.accessResource(animDataPc);
    config.linkCapability(VertexCapability::kAnimIndex,
                          code.makeMemberAccess(animData, "animation"), uint{},
                          { animDataPc });
    config.linkCapability(VertexCapability::kAnimKeyframes,
                          code.makeMemberAccess(animData, "keyframes"), uvec2{},
                          { animDataPc });
    config.linkCapability(VertexCapability::kAnimFrameWeight,
                          code.makeMemberAccess(animData, "keyframeWeigth"), float{},
                          { animDataPc });

    // Create the descriptor config
    MaterialRuntimeConfig conf;
    conf.descriptorInfos.try_emplace("global_data", 0, true);
    conf.descriptorInfos.try_emplace("asset_registry", 1, true);
    conf.pushConstantIds = {
        { DrawablePushConstIndex::eModelMatrix, modelPc },
        { DrawablePushConstIndex::eMaterialData, matIndexPc },
        { DrawablePushConstIndex::eAnimationData, animDataPc },
    };

    return { std::move(config), conf };
}

} // namespace trc
