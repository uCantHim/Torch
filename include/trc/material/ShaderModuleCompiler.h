#pragma once

#include <cassert>

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "ShaderOutputInterface.h"
#include "ShaderResourceInterface.h"
#include "ShaderModuleBuilder.h"
#include "trc/Types.h"

namespace trc
{
    /**
     * Holds information about a compiled shader module.
     */
    struct ShaderModule : ShaderResources
    {
        ShaderModule(const ShaderModule&) = default;
        ShaderModule(ShaderModule&&) = default;
        ShaderModule& operator=(const ShaderModule&) noexcept = default;
        ShaderModule& operator=(ShaderModule&&) noexcept = default;
        ~ShaderModule() noexcept = default;

        auto getGlslCode() const -> const std::string&;

    private:
        friend class ShaderModuleCompiler;

        ShaderModule(std::string shaderCode, ShaderResources resourceInfo);

        std::string shaderGlslCode;
    };

    class ShaderModuleCompiler
    {
    public:
        /**
         * @brief Compile a full shader module
         *
         * Compile resource requirements, function definitions, and output value
         * declarations into a shader module.
         *
         * Queries or creates a function "main" and appends output code
         * (assignments, function calls, ...) to it's block.
         */
        static auto compile(const ShaderOutputInterface& output,
                            ShaderModuleBuilder builder,
                            const ShaderCapabilityConfig& caps)
            -> ShaderModule;

    private:
        static auto compileSettings(const ShaderModuleBuilder::Settings& settings) -> std::string;
    };
} // namespace trc
