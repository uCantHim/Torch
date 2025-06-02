#include "trc/material/MaterialProgram.h"

#include <cstdlib>

#include <spirv/CompileSpirv.h>
#include <trc_util/StringManip.h>
#include <trc_util/Timer.h>

#include "trc/core/DeviceTask.h"
#include "trc/core/ResourceConfig.h"
#include "trc/core/Pipeline.h"
#include "trc/material/ShaderCache.h"



namespace trc
{

auto getShaderCacheFile() -> const char*
{
    if (auto env = std::getenv(TRC_SHADER_CACHE_ENV_NAME)) {
        return env;
    }
    return ".torch_material_shader_cache";
}

auto getShaderCache() -> ShaderCache&
{
    thread_local ShaderCache cache{ getShaderCacheFile() };
    return cache;
}

auto makePipelineLayout(const shader::ShaderProgramData& program)
    -> PipelineLayoutTemplate
{
    // Convert push constant ranges to PipelineLayoutTemplate's format
    std::vector<PipelineLayoutTemplate::PushConstant> pushConstants;
    pushConstants.reserve(program.pcRangesPerStage.size());
    for (const auto& [stage, range] : program.pcRangesPerStage)
    {
        pushConstants.push_back({
            .range=range,
            .defaultValue=std::nullopt
        });
    }

    // Convert descriptors to PipelineLayoutTemplate's format
    std::vector<PipelineLayoutTemplate::Descriptor> descriptors;
    descriptors.reserve(program.descriptorSets.size());
    for (const auto& desc : program.descriptorSets) {
        descriptors.push_back(PipelineLayoutTemplate::Descriptor{ {desc.name}, true });
    }

    return PipelineLayoutTemplate{ descriptors, pushConstants };
}

auto makeMaterialProgram(
    const shader::ShaderProgramData& data,
    const PipelineDefinitionData& pipeline,
    const RenderPassDefinition& renderPass,
    u_ptr<shaderc::CompileOptions> opts)
    -> std::expected<u_ptr<MaterialProgram>, ShaderCompileError>
{
    try {
        return std::make_unique<MaterialProgram>(data, pipeline, renderPass, std::move(opts));
    }
    catch (const ShaderCompileError& err) {
        return std::unexpected(err);
    }
}

auto shaderStageToShaderKind(vk::ShaderStageFlagBits stage) -> shaderc_shader_kind
{
    switch (stage)
    {
    case vk::ShaderStageFlagBits::eVertex: return shaderc_shader_kind::shaderc_vertex_shader;
    case vk::ShaderStageFlagBits::eGeometry: return shaderc_shader_kind::shaderc_geometry_shader;
    case vk::ShaderStageFlagBits::eTessellationControl: return shaderc_shader_kind::shaderc_tess_control_shader;
    case vk::ShaderStageFlagBits::eTessellationEvaluation: return shaderc_shader_kind::shaderc_tess_evaluation_shader;
    case vk::ShaderStageFlagBits::eFragment: return shaderc_shader_kind::shaderc_fragment_shader;

    case vk::ShaderStageFlagBits::eTaskEXT: return shaderc_shader_kind::shaderc_task_shader;
    case vk::ShaderStageFlagBits::eMeshEXT: return shaderc_shader_kind::shaderc_mesh_shader;

    case vk::ShaderStageFlagBits::eRaygenKHR: return shaderc_shader_kind::shaderc_raygen_shader;
    case vk::ShaderStageFlagBits::eIntersectionKHR: return shaderc_shader_kind::shaderc_intersection_shader;
    case vk::ShaderStageFlagBits::eMissKHR: return shaderc_shader_kind::shaderc_miss_shader;
    case vk::ShaderStageFlagBits::eAnyHitKHR: return shaderc_shader_kind::shaderc_anyhit_shader;
    case vk::ShaderStageFlagBits::eClosestHitKHR: return shaderc_shader_kind::shaderc_closesthit_shader;
    case vk::ShaderStageFlagBits::eCallableKHR: return shaderc_shader_kind::shaderc_callable_shader;

    case vk::ShaderStageFlagBits::eCompute: return shaderc_shader_kind::shaderc_compute_shader;

    case vk::ShaderStageFlagBits::eAll:
    case vk::ShaderStageFlagBits::eAllGraphics:
    case vk::ShaderStageFlagBits::eSubpassShadingHUAWEI:
    case vk::ShaderStageFlagBits::eClusterCullingHUAWEI:
        return shaderc_shader_kind::shaderc_glsl_infer_from_source;
    }

    std::unreachable();
}

/**
 * @return SPIR-V code, or an error string.
 */
auto compileShader(
    vk::ShaderStageFlagBits shaderStage,
    const std::string& glslCode,
    const shaderc::CompileOptions& opts)
    -> std::expected<std::vector<ui32>, std::string>
{
    Timer timer;

    // Check if the SPIR-V result is already cached.
    if (auto cached = getShaderCache().query(glslCode))
    {
        log::debug << "[MaterialProgram] Found cached SPIR-V code for shader stage "
                   << vk::to_string(shaderStage)
                   << " (lookup time: " << timer.reset() << "ms).";
        return std::move(cached->spirvCode);
    }

    // SPIR-V code is not cached, compile it now.
    timer.reset();
    const auto result = spirv::generateSpirv(
        glslCode,
        "<trc-material-generated-shader>",
        opts,
        shaderStageToShaderKind(shaderStage)
    );

    if (result.GetCompilationStatus() != shaderc_compilation_status_success)
    {
        static constexpr auto numDigits = [](size_t n) {
            size_t res = 1;
            while ((n /= 10) > 0) ++res;
            return res;
        };
        auto printWithLineNumbers = [](const std::string& str) -> std::generator<std::string> {
            const auto lines = util::splitString(str, '\n');
            const auto width = numDigits(lines.size() - 1);
            for (auto [i, line] : std::views::enumerate(lines)) {
                co_yield std::format(" {:<{}} | {}", i + 1, width, line);
            }
        };

        log::error << "[In makeMaterialProgram]: Unable to compile shader code for stage "
                   << vk::to_string(shaderStage) << " to SPIRV: " << result.GetErrorMessage() << "\n"
                   << "  >>> Tried to compile the following shader code:\n"
                   << "\n+++++ START SHADER CODE +++++\n"
                   << std::ranges::to<std::string>(
                           std::views::join_with(printWithLineNumbers(glslCode), '\n'))
                   << "\n+++++ END SHADER CODE +++++";
        return std::unexpected(result.GetErrorMessage());
    }

    // Log some info
    const auto time = timer.reset();
    log::info << "[MaterialProgram] Compiled GLSL code for " << vk::to_string(shaderStage)
              << " stage to SPIRV in " << time << "ms.";

    // Create result and cache it
    std::vector<ui32> spirv{ result.begin(), result.end() };
    getShaderCache().store(glslCode, { spirv, std::chrono::system_clock::now() });
    return spirv;
}

MaterialProgram::MaterialProgram(
    const shader::ShaderProgramData& data,
    const PipelineDefinitionData& _pipelineConfig,
    const RenderPassDefinition& renderPass,
    u_ptr<shaderc::CompileOptions> compileOptions)
    :
    pipelineConfig(_pipelineConfig),
    layout(PipelineRegistry::registerPipelineLayout(makePipelineLayout(data))),
    rootRuntime(nullptr)
{
    assert_arg(compileOptions != nullptr);

    // Create shader program
    ProgramDefinitionData program;
    for (const auto& [stage, glsl] : data.glslCode)
    {
        auto spirv = compileShader(stage, glsl, *compileOptions);
        if (spirv) {
            program.stages.emplace(stage, ProgramDefinitionData::ShaderStage{ std::move(*spirv) });
        }
        else {
            throw ShaderCompileError("[In MaterialProgram::MaterialProgram]:"
                                     " Shader compile error: " + spirv.error());
        }
    }

    // Load and set specialization constants
    for (const auto& [stageType, specs] : data.specConstants)
    {
        auto& stage = program.stages.at(stageType);
        for (const auto& [specIdx, specValue] : specs)
        {
            // Store the value provider (in case it wants to keep some data alive)
            runtimeValues.emplace_back(specValue);

            // Set specialization constant
            const auto data = specValue->loadData();
            assert(data.size() == specValue->getType().size());

            stage.specConstants.set(specIdx, data.data(), data.size());
        }
    }

    // Create pipeline
    pipeline = PipelineRegistry::registerPipeline(
        PipelineTemplate{ program, pipelineConfig },
        layout,
        renderPass
    );

    rootRuntime = std::make_unique<MaterialRuntime>(data, *this);
}

auto MaterialProgram::getPipelineConfig() const -> const PipelineDefinitionData&
{
    return pipelineConfig;
}

auto MaterialProgram::getPipeline() const -> Pipeline::ID
{
    return pipeline;
}

auto MaterialProgram::getRuntime() const -> s_ptr<MaterialRuntime>
{
    return rootRuntime;
}

auto MaterialProgram::cloneRuntime() const -> u_ptr<MaterialRuntime>
{
    return std::make_unique<MaterialRuntime>(*rootRuntime);
}



MaterialRuntime::MaterialRuntime(const shader::ShaderProgramData& program, MaterialProgram& prog)
    :
    ShaderProgramRuntime(program),
    pipeline(prog.getPipeline())
{
}

void MaterialRuntime::bind(vk::CommandBuffer cmdBuf, DeviceExecutionContext& ctx)
{
    auto& p = ctx.resources().getPipeline(pipeline);
    p.bind(cmdBuf, ctx.resources());
    shader::ShaderProgramRuntime::uploadPushConstantDefaultValues(cmdBuf, *p.getLayout());
}

auto MaterialRuntime::getPipeline() const -> Pipeline::ID
{
    return pipeline;
}

} // namespace trc
