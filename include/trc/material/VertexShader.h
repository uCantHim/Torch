#pragma once

#include "ShaderCapabilities.h"
#include "ShaderModuleBuilder.h"
#include "ShaderModuleCompiler.h"
#include "ShaderResourceInterface.h"

namespace trc
{
    struct VertexCapability
    {
        static constexpr Capability kPosition{ "vert_vertexPosition" };
        static constexpr Capability kNormal{ "vert_vertexNormal" };
        static constexpr Capability kTangent{ "vert_vertexTangent" };
        static constexpr Capability kUV{ "vert_vertexUV" };

        static constexpr Capability kBoneIndices{ "vert_boneIndices" };
        static constexpr Capability kBoneWeights{ "vert_boneWeights" };

        static constexpr Capability kModelMatrix{ "vert_modelMatrix" };
        static constexpr Capability kViewMatrix{ "vert_viewMatrix" };
        static constexpr Capability kProjMatrix{ "vert_projMatrix" };

        static constexpr Capability kAnimIndex{ "vert_animIndex" };
        static constexpr Capability kAnimKeyframes{ "vert_animKeyframes" };
        static constexpr Capability kAnimFrameWeight{ "vert_animFrameWeight" };
        static constexpr Capability kAnimMetaBuffer{ "vert_animMetaBuffer" };
        static constexpr Capability kAnimDataBuffer{ "vert_animDataBuffer" };
    };

    enum DrawablePushConstIndex : ui32
    {
        eMaterialData,
        eModelMatrix,
        eAnimationData,
    };

    class VertexModule
    {
    public:
        explicit VertexModule(bool animated);

        auto build(const ShaderModule& fragment) && -> ShaderModule;

    private:
        static auto makeVertexCapabilityConfig() -> ShaderCapabilityConfig;

        ShaderModuleBuilder builder;

        std::unordered_map<Capability, code::Value> fragmentInputProviders;
    };
} // namespace trc
