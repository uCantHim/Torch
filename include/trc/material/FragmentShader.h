#pragma once

#include "ShaderCapabilities.h"
#include "ShaderModuleBuilder.h"
#include "ShaderOutputNode.h"
#include "ShaderModuleCompiler.h"

namespace trc
{
    struct FragmentCapability
    {
        static constexpr Capability kVertexWorldPos{ "frag_vertexWorldPos" };
        static constexpr Capability kVertexNormal{ "frag_vertexNormal" };
        static constexpr Capability kVertexUV{ "frag_vertexUV" };
        static constexpr Capability kTangentToWorldSpaceMatrix{ "frag_tangentToWorld" };

        static constexpr Capability kCameraWorldPos{ "frag_cameraWorldPos" };

        static constexpr Capability kTime{ "frag_currentTime" };
        static constexpr Capability kTimeDelta{ "frag_frameTime" };

        static constexpr Capability kTextureSample{ "frag_textureSample" };

        static constexpr Capability kNextFragmentListIndex{ "frag_allocFragListIndex" };
        static constexpr Capability kMaxFragmentListIndex{ "frag_maxFragListIndex" };
        static constexpr Capability kFragmentListHeadPointerImage{ "frag_fragListPointerImage" };
        static constexpr Capability kFragmentListBuffer{ "frag_fragListBuffer" };
        static constexpr Capability kShadowMatrices{ "frag_shadowMatrixBuffer" };
        static constexpr Capability kLightBuffer{ "frag_lightDataBuffer" };
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
     *     FragmentModule::Parameter::eColor,
     *     builder.makeConstant(vec4{ 1, 0.5, 0, 1.0f })
     * );
     *
     * auto shaderModule = frag.build(std::move(builder), false);
     * ```
     */
    class FragmentModule
    {
    public:
        static constexpr size_t kNumParams{ 6 };

        enum class Parameter : size_t
        {
            eColor,
            eNormal,
            eSpecularFactor,
            eRoughness,
            eMetallicness,
            eEmissive,
        };

        FragmentModule() = default;

        /**
         * @brief Set a value for one of the module's output parameters
         *
         * @param Parameter param   The parameter for which to set a value.
         * @param code::Value value A shader code expression that calculates a
         *                          value for `param`.
         */
        void setParameter(Parameter param, code::Value value);

        /**
         * @brief Compile the module description to a shader module
         *
         * @param ShaderModuleBuilder moduleCode The code builder with which
         *        the shader code used in `FragmentModule::setParameter` was
         *        generated.
         * @param bool transparent An additional setting for the fragment
         *        shader. Set to `true` if the shader is used for transparent
         *        objects.
         */
        auto build(ShaderModuleBuilder moduleCode, bool transparent) -> ShaderModule;

    private:
        void fillDefaultValues(ShaderModuleBuilder& builder);

        std::array<std::optional<ParameterID>, kNumParams> parameters;
        ShaderOutputNode output;
    };
} // namespace trc
