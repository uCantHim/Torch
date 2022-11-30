#pragma once

#include "MaterialRuntime.h"
#include "ShaderCapabilities.h"
#include "ShaderModuleBuilder.h"
#include "ShaderModuleCompiler.h"
#include "ShaderResourceInterface.h"

namespace trc
{
    struct VertexCapability
    {
        static constexpr Capability kPosition{ "vertexPosition" };
        static constexpr Capability kNormal{ "vertexNormal" };
        static constexpr Capability kTangent{ "vertexTangent" };
        static constexpr Capability kUV{ "vertexUV" };

        static constexpr Capability kBoneIndices{ "boneIndices" };
        static constexpr Capability kBoneWeights{ "boneWeights" };

        static constexpr Capability kModelMatrix{ "modelMatrix" };
        static constexpr Capability kViewMatrix{ "viewMatrix" };
        static constexpr Capability kProjMatrix{ "projMatrix" };

        static constexpr Capability kAnimIndex{ "animIndex" };
        static constexpr Capability kAnimKeyframes{ "animKeyframes" };
        static constexpr Capability kAnimFrameWeight{ "animFrameWeight" };
        static constexpr Capability kAnimMetaBuffer{ "animMetaBuffer" };
        static constexpr Capability kAnimDataBuffer{ "animDataBuffer" };
    };

    enum DrawablePushConstIndex : ui32
    {
        eMaterialData,
        eModelMatrix,
        eAnimationData,
    };

    class VertexShaderBuilder
    {
    public:
        VertexShaderBuilder(ShaderModule fragmentResult,
                            bool animated);

        auto buildVertexShader() -> std::pair<ShaderModule, MaterialRuntimeConfig>;

    private:
        static auto makeVertexCapabilityConfig()
            -> std::pair<ShaderCapabilityConfig, MaterialRuntimeConfig>;

        ShaderModule fragment;

        std::pair<ShaderCapabilityConfig, MaterialRuntimeConfig> configs;
        ShaderModuleBuilder builder;

        std::unordered_map<Capability, code::Value> fragCapabilityProviders;
    };
} // namespace trc
