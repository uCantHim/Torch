#include "trc/material/VertexShader.h"

#include <initializer_list>
#include <unordered_map>



namespace trc
{

class GlPosition : public ShaderFunction
{
public:
    GlPosition()
        :
        ShaderFunction(
            "calcGlPosition",
            FunctionType{ { vec4{} }, vec4{} }
        )
    {}

    void build(ShaderModuleBuilder& builder, std::vector<code::Value> args) override
    {
        auto viewproj = builder.makeMul(
            builder.makeCapabilityAccess(VertexCapability::kProjMatrix),
            builder.makeCapabilityAccess(VertexCapability::kViewMatrix)
        );
        builder.makeReturn(builder.makeMul(viewproj, args[0]));
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
    MaterialCompileResult fragmentResult,
    bool animated)
    :
    fragment(std::move(fragmentResult)),
    builder(makeVertexCapabilityConfig())
{
    fragCapabilityProviders = {
        {
            FragmentCapability::kVertexWorldPos,
            [this, animated]() -> code::Value
            {
                auto objPos = builder.makeCapabilityAccess(VertexCapability::kPosition);
                auto modelMat = builder.makeCapabilityAccess(VertexCapability::kModelMatrix);
                auto objPos4 = builder.makeCall<ToVec4>({ objPos, builder.makeConstant(1.0f) });
                if (animated) {
                    objPos4 = builder.makeCall<ApplyAnimation>({ objPos4 });
                }

                auto worldPos = builder.makeMul(objPos4, modelMat);
                return worldPos;
            }()
        },
        {
            FragmentCapability::kVertexNormal,
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

auto VertexShaderBuilder::buildVertexShader() -> MaterialCompileResult
{
    MaterialOutputNode vertNode;
    for (const auto& out : fragment.getRequiredShaderInputs())
    {
        auto output = vertNode.addOutput(out.location, out.type);
        auto param = vertNode.addParameter(out.type);
        vertNode.linkOutput(param, output, "");

        auto inputNode = fragCapabilityProviders.at(out.capability);
        vertNode.setParameter(param, inputNode);
    }

    // Always add the gl_Position output
    const auto glPosOut = vertNode.addBuiltinOutput("gl_Position");
    const auto glPosParam = vertNode.addParameter(vec4{});
    vertNode.linkOutput(glPosParam, glPosOut);
    vertNode.setParameter(
        glPosParam,
        builder.makeCall<GlPosition>(
            { fragCapabilityProviders.at(FragmentCapability::kVertexWorldPos) }
        )
    );

    MaterialCompiler vertCompiler;
    return vertCompiler.compile(vertNode, builder);
}

auto VertexShaderBuilder::makeVertexCapabilityConfig() -> ShaderCapabilityConfig
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

    auto pushConstants = config.addResource(ShaderCapabilityConfig::PushConstant{
        "mat4 modelMatrix;\n"
        "uint materialIndex;\n"
        "AnimationPushConstantData animData;"
    });
    config.addShaderInclude(pushConstants, util::Pathlet("animation.glsl"));
    config.addMacro(pushConstants, "BONE_INDICES_INPUT_LOCATION", "4");
    config.addMacro(pushConstants, "BONE_WEIGHTS_INPUT_LOCATION", "5");
    config.addMacro(pushConstants, "ASSET_DESCRIPTOR_SET_BINDING", "1");

    auto vPos     = config.addResource(ShaderCapabilityConfig::ShaderInput{ vec3{} });
    auto vNormal  = config.addResource(ShaderCapabilityConfig::ShaderInput{ vec3{} });
    auto vUV      = config.addResource(ShaderCapabilityConfig::ShaderInput{ vec2{} });
    auto vTangent = config.addResource(ShaderCapabilityConfig::ShaderInput{ vec3{} });

    config.linkCapability(VertexCapability::kPosition, vPos, vec3{});
    config.linkCapability(VertexCapability::kNormal, vNormal, vec3{});
    config.linkCapability(VertexCapability::kTangent, vTangent, vec3{});
    config.linkCapability(VertexCapability::kUV, vUV, vec2{});

    // Model matrix
    auto pc = config.accessResource(pushConstants);
    config.linkCapability(
        VertexCapability::kModelMatrix,
        code.makeMemberAccess(config.accessResource(pushConstants), "modelMatrix"),
        mat4{},
        { pushConstants }
    );

    // Camera matrices
    auto camera = config.accessResource(cameraMatrices);
    config.linkCapability(VertexCapability::kViewMatrix,
                          code.makeMemberAccess(camera, "viewMatrix"), mat4{},
                          { cameraMatrices });
    config.linkCapability(VertexCapability::kProjMatrix,
                          code.makeMemberAccess(camera, "projMatrix"), mat4{},
                          { cameraMatrices });

    // Animation data
    config.linkCapability(VertexCapability::kAnimIndex,
                          code.makeMemberAccess(pc, "animData.animation"), uint{},
                          { pushConstants });
    config.linkCapability(VertexCapability::kAnimKeyframes,
                          code.makeMemberAccess(pc, "animData.keyframes"), uvec2{},
                          { pushConstants });
    config.linkCapability(VertexCapability::kAnimFrameWeight,
                          code.makeMemberAccess(pc, "animData.keyframeWeigth"), float{},
                          { pushConstants });

    return config;
}

} // namespace trc
