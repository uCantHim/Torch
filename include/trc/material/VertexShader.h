#pragma once

#include "trc/material/shader/Capability.h"
#include "trc/material/shader/ShaderModuleBuilder.h"
#include "trc/material/shader/ShaderModule.h"
#include "trc/material/shader/ShaderOutputInterface.h"

namespace trc
{
    /**
     * Capabilities for internal use in the vertex shader implementation.
     */
    namespace VertexCapability
    {
        using shader::Capability;

        inline const Capability kPosition{ "vert_vertexPosition" };
        inline const Capability kNormal{ "vert_vertexNormal" };
        inline const Capability kTangent{ "vert_vertexTangent" };
        inline const Capability kUV{ "vert_vertexUV" };

        inline const Capability kBoneIndices{ "vert_boneIndices" };
        inline const Capability kBoneWeights{ "vert_boneWeights" };

        inline const Capability kModelMatrix{ "vert_modelMatrix" };
        inline const Capability kViewMatrix{ "vert_viewMatrix" };
        inline const Capability kProjMatrix{ "vert_projMatrix" };

        inline const Capability kAnimIndex{ "vert_animIndex" };
        inline const Capability kAnimKeyframes{ "vert_animKeyframes" };
        inline const Capability kAnimFrameWeight{ "vert_animFrameWeight" };
        inline const Capability kAnimMetaBuffer{ "vert_animMetaBuffer" };
        inline const Capability kAnimDataBuffer{ "vert_animDataBuffer" };
    };

    enum DrawablePushConstIndex : ui32
    {
        eMaterialData,
        eModelMatrix,
        eAnimationData,
    };

    struct VertexModuleCreateInfo
    {
        bool animated{ false };
    };

    class VertexModule
    {
    public:
        explicit VertexModule(const VertexModuleCreateInfo& createInfo);

        auto buildOutputs(shader::ShaderModuleBuilder& builder,
                          const std::vector<trc::shader::ShaderResourceInterface::ShaderInputInfo>& requiredOutputs)
            -> shader::ShaderOutputInterface;

        auto build(const shader::ShaderModule& fragment) && -> shader::ShaderModule;

        static auto makeCapabilityConfig(const VertexModuleCreateInfo& config)
            -> u_ptr<shader::CapabilityConfig>;

    private:
        /**
         * Declare capabilities for *internal use* in the vertex module.
         */
        static auto makeInputCapabilityConfig() -> u_ptr<shader::CapabilityConfig>;

        /**
         * Create a module builder with a capability config that defines the
         * MaterialCapabilities that the vertex shader can implement as outputs.
         */
        static auto makeCapabilityConfigWithOutputs(const VertexModuleCreateInfo& createInfo)
            -> u_ptr<shader::CapabilityConfig>;

        /**
         * TODO: This is how a capability config for a virtual shader stage
         * "VertexInput" could be implemented and used in automatic shader stage
         * linking.
         */
        static auto __makeVertexInputCapabilityConfig() -> u_ptr<shader::CapabilityConfig>;

        const VertexModuleCreateInfo config;
    };
} // namespace trc
