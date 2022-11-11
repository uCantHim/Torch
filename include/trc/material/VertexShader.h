#pragma once

#include "MaterialFunction.h"
#include "MaterialGraph.h"
#include "ShaderCapabilities.h"
#include "ShaderResourceInterface.h"
#include "MaterialCompiler.h"

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

        static constexpr Capability kAnimIndex{ "animIndex" };
        static constexpr Capability kAnimKeyframes{ "animKeyframes" };
        static constexpr Capability kAnimFrameWeight{ "animFrameWeight" };
    };

    class VertexShaderBuilder
    {
    public:
        VertexShaderBuilder(MaterialCompileResult fragmentResult,
                            bool animated);

        auto buildVertexShader() -> MaterialCompileResult;

    private:
        using FragmentCapabilityFactory = std::function<MaterialNode*(MaterialGraph&)>;

        static auto makeVertexCapabilityConfig() -> ShaderCapabilityConfig;
        auto getFragmentCapabilityValue(Capability fragCapability, BasicType type) -> MaterialNode*;

        const std::unordered_map<Capability, FragmentCapabilityFactory> fragmentCapabilityFactories;

        MaterialGraph graph;
        MaterialCompileResult fragment;

        std::unordered_map<Capability, MaterialNode*> values;
    };
} // namespace trc
