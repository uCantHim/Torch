#pragma once

#include "trc/material/shader/Capability.h"
#include "trc/material/shader/ShaderModuleBuilder.h"
#include "trc/material/shader/ShaderModule.h"
#include "trc/material/shader/ShaderOutputInterface.h"

namespace trc
{
    namespace code = shader::code;

    namespace VertexCapability
    {
        inline const shader::Capability kPosition{ "vert_vertexPosition" };
        inline const shader::Capability kNormal{ "vert_vertexNormal" };
        inline const shader::Capability kTangent{ "vert_vertexTangent" };
        inline const shader::Capability kUV{ "vert_vertexUV" };

        inline const shader::Capability kBoneIndices{ "vert_boneIndices" };
        inline const shader::Capability kBoneWeights{ "vert_boneWeights" };

        inline const shader::Capability kModelMatrix{ "vert_modelMatrix" };
        inline const shader::Capability kViewMatrix{ "vert_viewMatrix" };
        inline const shader::Capability kProjMatrix{ "vert_projMatrix" };

        inline const shader::Capability kAnimIndex{ "vert_animIndex" };
        inline const shader::Capability kAnimKeyframes{ "vert_animKeyframes" };
        inline const shader::Capability kAnimFrameWeight{ "vert_animFrameWeight" };
        inline const shader::Capability kAnimMetaBuffer{ "vert_animMetaBuffer" };
        inline const shader::Capability kAnimDataBuffer{ "vert_animDataBuffer" };
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

        auto buildOutputs(shader::ShaderModuleBuilder& builder,
                          const std::vector<trc::shader::ShaderResourceInterface::ShaderInputInfo>& requiredOutputs)
            -> shader::ShaderOutputInterface;

        auto build(const shader::ShaderModule& fragment) && -> shader::ShaderModule;

        static auto makeCapabilityConfig() -> shader::CapabilityConfig;
        static auto makeVertexInputCapabilityConfig() -> shader::CapabilityConfig;

    private:
        shader::ShaderModuleBuilder builder;

        std::unordered_map<shader::Capability, code::Value> fragmentInputProviders;
    };
} // namespace trc
