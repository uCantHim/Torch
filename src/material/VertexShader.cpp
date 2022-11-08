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

std::unordered_map<Capability, std::function<MaterialNode*(MaterialGraph&)>> graphFactories{
    { FragmentCapability::kVertexWorldPos, [](MaterialGraph& graph) -> MaterialNode* {
        auto objPos = graph.makeCapabilityAccess(VertexCapability::kPosition, vec3{});
        auto modelMat = graph.makeCapabilityAccess(VertexCapability::kModelMatrix, mat4{});

        auto objPos4 = graph.makeFunction(Function{
            "expandPosition",
            [](ShaderResourceInterface&) { return "return vec4(_0, 1.0f);"; },
            { vec3{} }, vec4{}
        }, { objPos });
        auto worldPos = graph.makeFunction(MatrixApplication<4>{}, { objPos4, modelMat });

        return worldPos;
    }},
    { FragmentCapability::kVertexNormal, [](MaterialGraph& graph) -> MaterialNode* {
        Function normalToWorldspace{
            "calcWorldspaceNormal",
            [](ShaderResourceInterface& res) {
                return "return normalize((transpose(inverse("
                       + res.queryCapability(VertexCapability::kModelMatrix)
                       + ")) * vec4(_0, 0.0f)).xyz);";
            },
            { vec3{} }, vec3{}
        };
        auto normalObjspace = graph.makeCapabilityAccess(VertexCapability::kNormal, vec3{});
        auto tangentObjspace = graph.makeCapabilityAccess(VertexCapability::kTangent, vec3{});
        auto normal = graph.makeFunction(Function{ normalToWorldspace }, { normalObjspace });
        auto tangent = graph.makeFunction(Function{ normalToWorldspace }, { tangentObjspace });
        auto bitangent = graph.makeFunction(Function{
            "calcBitangent",
            [](ShaderResourceInterface&) { return "return cross(_0, _1);"; },
            { vec3{}, vec3{} }, vec3{}
        }, { normal, tangent });

        auto tbn = graph.makeFunction(Function{
            "assembleMatrix",
            [](ShaderResourceInterface&) { return "return mat3(_0, _1, _2);"; },
            { vec3{}, vec3{}, vec3{} }, mat3{}
        }, { tangent, bitangent, normal });

        return tbn;
    }},
    { FragmentCapability::kVertexUV, [](MaterialGraph& graph) -> MaterialNode* {
        return graph.makeCapabilityAccess(VertexCapability::kUV, vec2{});
    }},
};

auto makeFragmentCapabilityComputation(Capability fragCapability, BasicType type, MaterialGraph& graph)
    -> MaterialNode*
{
    if (graphFactories.contains(fragCapability)) {
        return graphFactories.at(fragCapability)(graph);
    }
    return graph.makeConstant({ type, {{ std::byte(0) }} });
}



VertexShaderBuilder::VertexShaderBuilder(MaterialGraph& graph)
    :
    graph(graph)
{
}

auto VertexShaderBuilder::getFragmentCapabilityValue(
    Capability fragCapability,
    BasicType type
    ) -> MaterialNode*
{
    if (!values.contains(fragCapability))
    {
        auto node = makeFragmentCapabilityComputation(fragCapability, type, graph);
        values.try_emplace(fragCapability, node);
        return node;
    }

    assert(values.at(fragCapability) != nullptr);
    return values.at(fragCapability);
}

} // namespace trc
