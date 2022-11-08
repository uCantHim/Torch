#pragma once

#include "MaterialFunction.h"
#include "MaterialGraph.h"
#include "ShaderCapabilities.h"
#include "ShaderResourceInterface.h"

namespace trc
{
    struct VertexCapability
    {
        static constexpr Capability kPosition{ "vertexPosition" };
        static constexpr Capability kNormal{ "vertexNormal" };
        static constexpr Capability kTangent{ "vertexTangent" };
        static constexpr Capability kUV{ "vertexUV" };
        static constexpr Capability kModelMatrix{ "modelMatrix" };
        static constexpr Capability kViewMatrix{ "viewMatrix" };
        static constexpr Capability kProjMatrix{ "projMatrix" };
    };

    auto makeFragmentCapabilityComputation(Capability fragCapability,
                                           BasicType type,
                                           MaterialGraph& graph)
        -> MaterialNode*;

    class VertexShaderBuilder
    {
    public:
        explicit VertexShaderBuilder(MaterialGraph& graph);

        auto getFragmentCapabilityValue(Capability fragCapability, BasicType type) -> MaterialNode*;

    private:
        MaterialGraph& graph;
        std::unordered_map<Capability, MaterialNode*> values;
    };
} // namespace trc
