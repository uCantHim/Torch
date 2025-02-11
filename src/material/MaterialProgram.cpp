#include "trc/material/MaterialProgram.h"

#include <spirv/CompileSpirv.h>
#include <trc_util/StringManip.h>
#include <trc_util/Timer.h>

#include "trc/core/DeviceTask.h"
#include "trc/core/ResourceConfig.h"
#include "trc/core/Pipeline.h"



namespace trc
{

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

auto shaderStageToExtension(vk::ShaderStageFlagBits stage) -> std::string
{
    switch (stage)
    {
    case vk::ShaderStageFlagBits::eVertex: return ".vert";
    case vk::ShaderStageFlagBits::eGeometry: return ".geom";
    case vk::ShaderStageFlagBits::eTessellationControl: return ".tese";
    case vk::ShaderStageFlagBits::eTessellationEvaluation: return ".tesc";
    case vk::ShaderStageFlagBits::eFragment: return ".frag";

    case vk::ShaderStageFlagBits::eTaskEXT: return ".task";
    case vk::ShaderStageFlagBits::eMeshEXT: return ".mesh";

    case vk::ShaderStageFlagBits::eRaygenKHR: return ".rgen";
    case vk::ShaderStageFlagBits::eIntersectionKHR: return ".rint";
    case vk::ShaderStageFlagBits::eMissKHR: return ".rmiss";
    case vk::ShaderStageFlagBits::eAnyHitKHR: return ".rahit";
    case vk::ShaderStageFlagBits::eClosestHitKHR: return ".rchit";
    case vk::ShaderStageFlagBits::eCallableKHR: return ".rcall";

    case vk::ShaderStageFlagBits::eCompute: return ".comp";

    case vk::ShaderStageFlagBits::eAll:
    case vk::ShaderStageFlagBits::eAllGraphics:
    case vk::ShaderStageFlagBits::eSubpassShadingHUAWEI:
    case vk::ShaderStageFlagBits::eClusterCullingHUAWEI:
        return ".glsl";
    }

    assert(false);
    throw std::logic_error("");
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
    const auto result = spirv::generateSpirv(
        glslCode,
        "foo" + shaderStageToExtension(shaderStage),
        opts
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
                co_yield std::format("{:<{}} | {}", i + 1, width, line);
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

    return std::vector<ui32>{ result.begin(), result.end() };
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
        log::info << "Compiling GLSL code for " << vk::to_string(stage) << " stage to SPIRV...";

        Timer timer;
        if (auto spirv = compileShader(stage, glsl, *compileOptions))
        {
            log::info << "Compiled in " << timer.reset() << " ms.";
            program.stages.emplace(stage, ProgramDefinitionData::ShaderStage{ std::move(*spirv) });
        }
        else {
            throw ShaderCompileError("[In MaterialProgram::MaterialProgram]: Shader compile error.");
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
