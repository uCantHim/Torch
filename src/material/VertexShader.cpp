#include "trc/material/VertexShader.h"

#include <initializer_list>
#include <unordered_map>



namespace trc
{

class Function : public MaterialFunction
{
public:
    template<std::invocable<ShaderResourceInterface&> F>
    Function(std::string name, F&& func, std::initializer_list<BasicType> params, BasicType resultType)
        :
        MaterialFunction(Signature{
            std::move(name),
            [&]{
                std::vector<Param> result;
                for (ui32 i = 0; auto type : params) {
                    result.push_back({ "_" + std::to_string(i++), type });
                }
                return result;
            }(),
            Param{ "out", resultType }
        }),
        func(std::move(func))
    {
    }

    auto makeGlslCode(ShaderResourceInterface& resources) -> std::string override
    {
        return func(resources);
    }

private:
    std::function<std::string(ShaderResourceInterface&)> func;
};

class GlPosition : public MaterialFunction
{
public:
    GlPosition()
        :
        MaterialFunction(Signature{
            "calcGlPosition",
            { Param{ "worldPos", vec4{} } },
            Param{ "out", vec4{} }
        })
    {}

    auto makeGlslCode(ShaderResourceInterface& resources) -> std::string
    {
        return "return "
            + resources.queryCapability(VertexCapability::kProjMatrix)
            + " * " + resources.queryCapability(VertexCapability::kViewMatrix)
            + " * worldPos;";
    }
};

template<ui32 N>
class MatrixApplication : public MaterialFunction
{
public:
    using VecT = glm::vec<N, float>;
    using MatT = glm::mat<N, N, float>;

    MatrixApplication()
        :
        MaterialFunction(Signature{
            "applyMatrix",
            { Param{ "vector", VecT{} }, Param{ "matrix", MatT{} } },
            Param{ "out", VecT{} }
        })
    {}

    auto makeGlslCode(ShaderResourceInterface&) -> std::string override
    {
        return "return matrix * vector;";
    }
};

class ApplyAnimation : public MaterialFunction
{
public:
    ApplyAnimation()
        :
        MaterialFunction(Signature{
            "applyAnimationTransform",
            { Param{ "v", vec4{} }, },
            Param{ "out", vec4{} }
        })
    {}

    auto makeGlslCode(ShaderResourceInterface& resources) -> std::string override
    {
        return "return applyAnimation("
            + resources.queryCapability(VertexCapability::kAnimIndex)
            + ", v"
            + ", " + resources.queryCapability(VertexCapability::kAnimKeyframes)
            + ", " + resources.queryCapability(VertexCapability::kAnimFrameWeight)
            + ");";
    }
};

class ToVec4 : public MaterialFunction
{
public:
    explicit ToVec4(float w)
        :
        MaterialFunction(Signature{
            "toVec4",
            { Param{ "v", vec3{} }, },
            Param{ "out", vec4{} }
        }),
        w(w)
    {}

    auto makeGlslCode(ShaderResourceInterface&) -> std::string override {
        return "return vec4(v, " + std::to_string(w) + ");";
    }

private:
    const float w;
};



VertexShaderBuilder::VertexShaderBuilder(
    MaterialCompileResult fragmentResult,
    bool animated)
    :
    fragmentCapabilityFactories({
        { FragmentCapability::kVertexWorldPos, [animated](MaterialGraph& graph) -> MaterialNode* {
            auto objPos = graph.makeCapabilityAccess(VertexCapability::kPosition, vec3{});
            auto modelMat = graph.makeCapabilityAccess(VertexCapability::kModelMatrix, mat4{});
            auto objPos4 = graph.makeFunctionCall(ToVec4(1.0f), { objPos });
            if (animated) {
                objPos4 = graph.makeFunctionCall(ApplyAnimation{}, { objPos4 });
            }

            auto worldPos = graph.makeFunctionCall(MatrixApplication<4>{}, { objPos4, modelMat });

            return worldPos;
        }},
        { FragmentCapability::kVertexNormal, [animated](MaterialGraph& graph) -> MaterialNode* {
            Function normalToWorldspace{
                "calcWorldspaceNormal",
                [](ShaderResourceInterface& res) {
                    return "return normalize((transpose(inverse("
                           + res.queryCapability(VertexCapability::kModelMatrix)
                           + ")) * _0).xyz);";
                },
                { vec4{} }, vec3{}
            };
            auto normalObjspace = graph.makeCapabilityAccess(VertexCapability::kNormal, vec3{});
            auto tangentObjspace = graph.makeCapabilityAccess(VertexCapability::kTangent, vec3{});
            normalObjspace = graph.makeFunctionCall(ToVec4(0.0f), { normalObjspace });
            tangentObjspace = graph.makeFunctionCall(ToVec4(0.0f), { tangentObjspace });
            if (animated) {
                normalObjspace = graph.makeFunctionCall(ApplyAnimation{}, { normalObjspace });
                tangentObjspace = graph.makeFunctionCall(ApplyAnimation{}, { tangentObjspace });
            }

            auto normal = graph.makeFunctionCall(Function{ normalToWorldspace }, { normalObjspace });
            auto tangent = graph.makeFunctionCall(Function{ normalToWorldspace }, { tangentObjspace });
            auto bitangent = graph.makeFunctionCall(Function{
                "calcBitangent",
                [](ShaderResourceInterface&) { return "return cross(_0, _1);"; },
                { vec3{}, vec3{} }, vec3{}
            }, { normal, tangent });

            auto tbn = graph.makeFunctionCall(Function{
                "assembleMatrix",
                [](ShaderResourceInterface&) { return "return mat3(_0, _1, _2);"; },
                { vec3{}, vec3{}, vec3{} }, mat3{}
            }, { tangent, bitangent, normal });

            return tbn;
        }},
        { FragmentCapability::kVertexUV, [](MaterialGraph& graph) -> MaterialNode* {
            return graph.makeCapabilityAccess(VertexCapability::kUV, vec2{});
        }},
    }),
    fragment(std::move(fragmentResult))
{
}

auto VertexShaderBuilder::buildVertexShader() -> MaterialCompileResult
{
    MaterialOutputNode vertNode;
    for (const auto& out : fragment.getRequiredShaderInputs())
    {
        auto output = vertNode.addOutput(out.location, out.type);
        auto param = vertNode.addParameter(out.type);
        vertNode.linkOutput(param, output, "");

        auto inputNode = getFragmentCapabilityValue(out.capability, out.type);
        vertNode.setParameter(param, inputNode);
    }

    // Always add the gl_Position output
    const auto glPosOut = vertNode.addBuiltinOutput("gl_Position");
    const auto glPosParam = vertNode.addParameter(vec4{});
    vertNode.linkOutput(glPosParam, glPosOut);
    vertNode.setParameter(
        glPosParam,
        graph.makeFunctionCall(
            GlPosition{},
            { getFragmentCapabilityValue(FragmentCapability::kVertexWorldPos, vec4{}) }
        )
    );

    MaterialCompiler vertCompiler(makeVertexCapabilityConfig());
    return vertCompiler.compile(vertNode);
}

auto VertexShaderBuilder::getFragmentCapabilityValue(
    Capability fragCapability,
    BasicType type
    ) -> MaterialNode*
{
    if (!values.contains(fragCapability))
    {
        MaterialNode* node;
        if (fragmentCapabilityFactories.contains(fragCapability)) {
            node = fragmentCapabilityFactories.at(fragCapability)(graph);
        }
        else {
            node = graph.makeConstant({ type, {{ std::byte(0) }} });
        }

        values.try_emplace(fragCapability, node);
        return node;
    }

    assert(values.at(fragCapability) != nullptr);
    return values.at(fragCapability);
}

auto VertexShaderBuilder::makeVertexCapabilityConfig() -> ShaderCapabilityConfig
{
    ShaderCapabilityConfig config;

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
    config.addMacro(pushConstants, "ASSET_DESCRIPTOR_SET_BINDING", "0");

    auto vPos     = config.addResource(ShaderCapabilityConfig::ShaderInput{ vec3{} });
    auto vNormal  = config.addResource(ShaderCapabilityConfig::ShaderInput{ vec3{} });
    auto vUV      = config.addResource(ShaderCapabilityConfig::ShaderInput{ vec2{} });
    auto vTangent = config.addResource(ShaderCapabilityConfig::ShaderInput{ vec3{} });

    config.linkCapability(VertexCapability::kPosition, vPos);
    config.linkCapability(VertexCapability::kNormal, vNormal);
    config.linkCapability(VertexCapability::kTangent, vTangent);
    config.linkCapability(VertexCapability::kUV, vUV);

    config.linkCapability(VertexCapability::kModelMatrix, pushConstants);
    config.linkCapability(VertexCapability::kViewMatrix, cameraMatrices);
    config.linkCapability(VertexCapability::kProjMatrix, cameraMatrices);
    config.setCapabilityAccessor(VertexCapability::kModelMatrix, ".modelMatrix");
    config.setCapabilityAccessor(VertexCapability::kViewMatrix, ".viewMatrix");
    config.setCapabilityAccessor(VertexCapability::kProjMatrix, ".projMatrix");

    config.linkCapability(VertexCapability::kAnimIndex,       pushConstants);
    config.linkCapability(VertexCapability::kAnimKeyframes,   pushConstants);
    config.linkCapability(VertexCapability::kAnimFrameWeight, pushConstants);
    config.setCapabilityAccessor(VertexCapability::kAnimIndex,       ".animData.animation");
    config.setCapabilityAccessor(VertexCapability::kAnimKeyframes,   ".animData.keyframes");
    config.setCapabilityAccessor(VertexCapability::kAnimFrameWeight, ".animData.keyframeWeigth");

    return config;
}

} // namespace trc
