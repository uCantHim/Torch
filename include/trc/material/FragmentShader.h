#pragma once

#include "trc/material/MaterialShaderImpl.h"
#include "trc/material/shader/Capability.h"
#include "trc/material/shader/ShaderModule.h"
#include "trc/material/shader/ShaderModuleBuilder.h"
#include "trc/material/shader/ShaderOutputInterface.h"

namespace trc
{
    /**
     * A collection of capabilities intended to be used by shader code
     * implementing material calculations: 'user code' if you will.
     */
    namespace MaterialCapability
    {
        inline const shader::Capability kVertexWorldPos{ "vertexWorldPos" };
        inline const shader::Capability kVertexNormal{ "vertexNormal" };
        inline const shader::Capability kVertexUV{ "vertexUV" };
        inline const shader::Capability kTangentToWorldSpaceMatrix{ "tangentToWorld" };

        inline const shader::Capability kCameraWorldPos{ "cameraWorldPos" };

        inline const shader::Capability kTime{ "currentTime" };
        inline const shader::Capability kTimeDelta{ "frameTime" };

        /**
         * Gives access to an array of texture samplers. The array shall be
         * indexed via the index obtained from `TextureHandle::getDeviceIndex`.
         *
         * Use the `TextureSample` shader function as a default implementation.
         */
        inline const shader::Capability kTextureSample{ "trc_mat_textureSample" };
    } // namespace MaterialCapability

    /**
     * Capabilities for internal use in fragment shaders generated from material
     * descriptions.
     */
    namespace FragmentCapability
    {
        inline const shader::Capability kNextFragmentListIndex{ "frag_allocFragListIndex" };
        inline const shader::Capability kMaxFragmentListIndex{ "frag_maxFragListIndex" };
        inline const shader::Capability kFragmentListHeadPointerImage{ "frag_fragListPointerImage" };
        inline const shader::Capability kFragmentListBuffer{ "frag_fragListBuffer" };
        inline const shader::Capability kShadowMatrices{ "frag_shadowMatrixBuffer" };
        inline const shader::Capability kLightBuffer{ "frag_lightDataBuffer" };
    } // namespace FragmentCapability

    /**
     * Capabilities for internal use in callable shaders generated from material
     * descriptions.
     */
    namespace RayHitCapability
    {
        inline const shader::Capability kBarycentricCoords{ "rcall_baryCoords" };
        inline const shader::Capability kGeometryIndex{ "rcall_geoIndex" };

        inline const shader::Capability kOutColor{ "rcall_colorOutput" };
    } // namespace RayHitCapability

    struct FragmentModuleCreateInfo
    {
        bool transparent{ false };
    };

    /**
     * @brief Torch's implementation of a configurable fragment shader
     *
     * The fragment module has a set of output parameters that are passed to the
     * lighting algorithm. The fragment shader calculates values for these
     * parameters.
     *
     * Set shader code expressions as parameters with the function
     * `FragmentModule::setParameter`.
     *
     * # Example
     *
     * ```cpp
     * ShaderModuleBuilder builder{ myConfig };
     * FragmentModule frag;
     * frag.setParameter(
     *     FragmentModule::Out::color,
     *     builder.makeConstant(vec4{ 1, 0.5, 0, 1.0f })
     * );
     *
     * auto shaderModule = frag.build(std::move(builder), false);
     * ```
     */
    class FragmentModule : public MaterialShaderImpl
    {
    public:
        // Built-in outputs of the fragment module.
        struct Out
        {
            static constexpr OutputParameter color{ "trc_frag_color" };
            static constexpr OutputParameter normal{ "trc_frag_normal" };
            static constexpr OutputParameter specularFactor{ "trc_frag_specularFactor" };
            static constexpr OutputParameter roughness{ "trc_frag_roughness" };
            static constexpr OutputParameter metallicness{ "trc_frag_metallicness" };
            static constexpr OutputParameter emissive{ "trc_frag_emissive" };
        };

        explicit
        FragmentModule(const FragmentModuleCreateInfo& createInfo);

        auto makeCapabilityConfig() -> u_ptr<shader::CapabilityConfig> override;

        auto makeOutputs(shader::ShaderModuleBuilder& builder)
            -> shader::ShaderOutputInterface override;

        /**
         * @brief Compile the module description to a fragment shader module
         *
         * @param ShaderModuleBuilder moduleCode The code builder with which
         *        the shader code used in `FragmentModule::setParameter` was
         *        generated.
         * @param bool transparent An additional setting for the fragment
         *        shader. Set to `true` if the shader is used for transparent
         *        objects.
         *
         * @throw std::invalid_argument if a required parameter has not been set
         *                              beforehand.
         */
        auto build(shader::ShaderModuleBuilder moduleCode)
            -> shader::ShaderModule;

        /**
         * @brief Compile the module description to a closest-hit shader module
         *
         * @param ShaderModuleBuilder moduleCode The code builder with which
         *        the shader code used in `FragmentModule::setParameter` was
         *        generated.
         */
        auto buildClosesthitShader(shader::ShaderModuleBuilder builder) -> shader::ShaderModule;

    private:
        void fillDefaultValues(shader::ShaderModuleBuilder& builder);

        FragmentModuleCreateInfo config;
    };
} // namespace trc
