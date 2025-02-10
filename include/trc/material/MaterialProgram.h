#pragma once

#include <trc_util/Exception.h>

#include "trc/core/PipelineLayoutTemplate.h"
#include "trc/core/PipelineTemplate.h"
#include "trc/core/RenderPassRegistry.h"
#include "trc/material/TorchMaterialSettings.h"
#include "trc/material/shader/ShaderProgram.h"

namespace trc
{
    struct DeviceExecutionContext;

    class MaterialRuntime;
    class MaterialProgram;

    /**
     * @brief Thrown from MaterialProgram's constructor if SPIR-V compilation
     *        fails.
     */
    class ShaderCompileError : public Exception
    {
    public:
        ShaderCompileError(const std::string& errorMsg) : Exception(errorMsg) {}
    };

    /**
     * @brief Create a pipeline layout configuration for the shader program
     *
     * The program knows everything that's necessary to create a pipeline
     * layout compatible to it.
     *
     * @return PipelineLayoutTemplate
     */
    auto makePipelineLayout(const shader::ShaderProgramData& program)
        -> PipelineLayoutTemplate;

    class MaterialProgram
    {
    public:
        /**
         * Create runtime resources for a shader program, including Vulkan
         * pipeline, pipeline layout, and SPIR-V shader code.
         *
         * Compiling GLSL to SPIR-V may take a while (hundrets of milliseconds);
         * it may be a good idea to execute this constructor in parallel to
         * other work.
         *
         * @param data           A shader program.
         * @param pipelineConfig Parameters for the resulting graphics pipeline.
         * @param renderPass     Render pass compatibility information.
         * @param compileOptions Compile options for shader code. Shall not be
         *                       `nullptr`. Note that this also defines a file
         *                       includer strategy. The default value does not
         *                       search any include paths.
         *                       NOTE to self: This has to be a unique ptr
         *                       because moving `shaderc::CompileOptions` seems
         *                       to be causing crashes in shaderc.
         *
         * @throw std::invalid_argument if `compileOptions` is `nullptr`.
         * @throw ShaderCompileError if SPIR-V compilation fails.
         */
        MaterialProgram(const shader::ShaderProgramData& data,
                        const PipelineDefinitionData& pipelineConfig,
                        const RenderPassDefinition& renderPass,
                        u_ptr<shaderc::CompileOptions> compileOptions = makeShaderCompileOptions());

        auto getPipeline() const -> Pipeline::ID;

        auto getRuntime() const -> s_ptr<MaterialRuntime>;
        auto cloneRuntime() const -> u_ptr<MaterialRuntime>;

    private:
        PipelineLayout::ID layout;
        Pipeline::ID pipeline;

        std::vector<s_ptr<shader::ShaderRuntimeConstant>> runtimeValues;  // Keep the objects alive

        // The default runtime for this shader program.
        s_ptr<MaterialRuntime> rootRuntime;
    };

    class MaterialRuntime : public shader::ShaderProgramRuntime
    {
    public:
        MaterialRuntime(const shader::ShaderProgramData& program, MaterialProgram& prog);

        /**
         * @brief Bind the pipeline object and upload default push constants.
         */
        void bind(vk::CommandBuffer cmdBuf, DeviceExecutionContext& ctx);

        auto getPipeline() const -> Pipeline::ID;

    private:
        Pipeline::ID pipeline;
    };
} // namespace trc
