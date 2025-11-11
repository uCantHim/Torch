#pragma once

#include <shader_tools/ShaderDocument.h>

#include "ShaderResourceInterface.h"
#include "trc/serial/shader_module.pb.h"

namespace trc::shader
{
    /**
     * Holds information about a compiled shader module.
     */
    struct ShaderModule : ShaderResourceInterface
    {
        ShaderModule(const ShaderModule&) = default;
        ShaderModule(ShaderModule&&) = default;
        ShaderModule& operator=(const ShaderModule&) noexcept = default;
        ShaderModule& operator=(ShaderModule&&) noexcept = default;
        ~ShaderModule() noexcept = default;

        /**
         * @brief Create a shader module.
         */
        ShaderModule(shader_edit::ShaderDocument shaderCode,
                     ShaderResourceInterface resourceInfo);

        /**
         * @return The code for the entire shader module. May contain unset
         *         variables, such as descriptor set index placeholders. The
         *         shader module has functions to query information about these.
         */
        auto getShaderCode() const -> const shader_edit::ShaderDocument&;

        auto serialize() const -> serial::ShaderModule;

        /**
         * @param des May be omitted by passing `nullptr`, but runtime constants
         *            will not be deserialized.
         */
        static auto deserialize(const serial::ShaderModule& mod,
                                ShaderRuntimeConstantDeserializer* des)
            -> ShaderModule;

    private:
        shader_edit::ShaderDocument shaderCode;
    };
} // namespace trc::shader
