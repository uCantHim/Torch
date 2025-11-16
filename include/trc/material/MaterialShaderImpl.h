#pragma once

#include <optional>
#include <string_view>
#include <unordered_map>

#include "trc/Types.h"
#include "trc/material/shader/ShaderModuleBuilder.h"
#include "trc/material/shader/ShaderOutputInterface.h"

namespace trc
{
    /**
     * @brief Engine implementation of a shader stage.
     */
    class MaterialShaderImpl
    {
    public:
        /**
         * Handle to a shader stage's output parameter.
         *
         * The canonical idiom is this:
         * ```
         * class MyVertexShader : public trc::MaterialShaderImpl
         * {
         * public:
         *     struct Out {
         *         static constexpr OutputParameter position{ "my_position" };
         *         static constexpr OutputParameter normal{ "my_normal" };
         *     };
         * };
         * ```
         *
         * So that it can be used like this:
         * ```
         * void configure(MyVertexShader& vert)
         * {
         *     vert.setParameter(MyVertexShader::Out::position, ...);
         *     ...
         * }
         * ```
         */
        struct OutputParameter
        {
            consteval OutputParameter(std::string_view str) : name(str) {}
            constexpr auto operator<=>(const OutputParameter&) const = default;

            std::string_view name;
        };

        virtual ~MaterialShaderImpl() noexcept = default;

        /**
         * Create a capability configuration that provides both internally used
         * capabilities and capabilities for external access, such as values
         * that can be passed to later shader stages.
         *
         * @return May not be `nullptr`.
         */
        virtual auto makeCapabilityConfig() -> u_ptr<shader::CapabilityConfig> = 0;

        /**
         * Create necessary shader outputs, including built-in outputs values
         * (e.g., gl_Position in a vertex shader) and user-defined outputs that
         * were defined via `setParameter`. The result can be further modified
         * to add additional outputs.
         *
         * This function may transform the semantical output parameters defined
         * by `OutputParameter` handles and configured via `setParameter` (e.g.,
         * albedo, roughness for a fragment shader) to the engine implementation
         * (e.g., use values for lighting calculations, or store values in a
         * g-buffer, ...).
         *
         * @param builder The builder used to build the shader module. Usually
         *                a builder created with the capability config returned
         *                by `makeCapabilityConfig`.
         */
        virtual auto makeOutputs(shader::ShaderModuleBuilder& builder)
            -> shader::ShaderOutputInterface = 0;

        /**
         * @brief Set a value for an output parameter.
         *
         * Overwrites any current value.
         */
        void setParameter(const OutputParameter& output, shader::code::Value value);

    protected:
        /**
         * @throw std::out_of_range if `param` does not have a value.
         */
        auto getParameterValue(const OutputParameter& param) -> shader::code::Value;

        auto tryGetParameterValue(const OutputParameter& param) -> std::optional<shader::code::Value>;

    private:
        struct Hash
        {
            auto operator()(const OutputParameter& id) const -> size_t {
                return std::hash<std::string_view>{}(id.name);
            }
        };

        std::unordered_map<OutputParameter, shader::code::Value, Hash> parameterValues;
    };
} // namespace trc
