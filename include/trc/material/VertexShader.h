#pragma once

#include "MaterialCompiler.h"
#include "ShaderCapabilities.h"
#include "ShaderModuleBuilder.h"
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
        static auto makeVertexCapabilityConfig() -> ShaderCapabilityConfig;

        MaterialCompileResult fragment;
        ShaderModuleBuilder builder;

        std::unordered_map<Capability, code::Value> fragCapabilityProviders;
    };
} // namespace trc
