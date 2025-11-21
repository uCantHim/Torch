#pragma once

#include <string_view>
using namespace std::string_view_literals;

#include "trc/material/MaterialShaderImpl.h"
#include "trc/material/shader/Capability.h"
#include "trc/material/shader/ShaderModuleBuilder.h"
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

    namespace DrawablePushConstIndex
    {
        constexpr auto eMaterialData = "trc:pc:material_data"sv;
        constexpr auto eModelMatrix = "trc:pc:model_matrix"sv;
        constexpr auto eAnimationData = "trc:pc:anim_data"sv;
    };

    struct VertexModuleCreateInfo
    {
        bool animated{ false };
    };

    class VertexModule : public MaterialShaderImpl
    {
    public:
        // Built-in outputs of the vertex module.
        struct Out
        {
            static constexpr OutputParameter vertexPosition{ "trc_vert_glPosition" };
        };

        explicit
        VertexModule(const VertexModuleCreateInfo& createInfo);

        auto makeOutputs(shader::ShaderModuleBuilder& builder)
            -> shader::ShaderOutputInterface override;

        auto makeCapabilityConfig() -> u_ptr<shader::CapabilityConfig> override {
            return makeCapabilityConfig(config);
        }

        static auto makeCapabilityConfig(const VertexModuleCreateInfo& config)
            -> u_ptr<shader::CapabilityConfig>;

    private:
        /**
         * Declare capabilities for *internal use* in the vertex module.
         */
        static auto makeInputCapabilityConfig() -> u_ptr<shader::CapabilityConfig>;

        /**
         * Declare internal capabilities as well as public capabilities that
         * define vertex shader outputs.
         */
        static auto makeCapabilityConfigWithOutputs(const VertexModuleCreateInfo& createInfo)
            -> u_ptr<shader::CapabilityConfig>;

        /**
         * TODO: This is how a capability config for a virtual shader stage
         * "VertexInput" could be implemented and used in automatic shader stage
         * linking.
         */
        static auto __makeVertexInputCapabilityConfig() -> u_ptr<shader::CapabilityConfig>;

        VertexModuleCreateInfo config;
    };
} // namespace trc
