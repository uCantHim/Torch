#include <iostream>
#include <fstream>

#include <cloth/cloth.h>
#include <cloth/parse_utils.h>
#include <cloth/parser.h>
#include <cloth/torch_impl.h>
#include <trc_util/StringManip.h>
#include <trc_util/Timer.h>
#include <trc_util/TypeUtils.h>

#include <trc/material/MaterialProgram.h>
#include <trc/DrawablePipelines.h>

int main(int argc, const char** argv)
{
    if (argc != 2) {
        std::cout << "Usage: cloth_multi_document FILE\n";
        return 1;
    }

    std::string filePath{ argv[1] };
    std::ifstream file{ filePath };

    trc::Timer time;
    const auto lines = trc::util::readLines(file);
    const auto res = cloth::compileMultiShader(
        cloth::parser::parseMultiDocument(lines),
        {
            { cloth::parser::ShaderStage::eVertex, std::make_shared<cloth::TorchVertexShaderImpl>(false) },
            { cloth::parser::ShaderStage::eFragment, std::make_shared<cloth::TorchImpl>() },
        }
    );

    if (res.hasErrors()) {
        std::cout << cloth::util::formatErrors(res.errors, lines, filePath);
    }

    std::cout << "Cloth compilation finished in " << time.reset() << "ms\n";
    std::cout << "Detected the following shader modules:\n";
    for (const auto& [stage, compileResult] : res.shaderStages) {
        std::cout << " - " << cloth::parser::to_string(stage) << " (compiled successfully)\n";
    }
    for (const auto& [stage, err] : res.shaderStageErrors)
    {
        std::cout << " (compile errors)\n";
        std::cout << cloth::util::formatErrors(err.errors, err.initialDocumentLines, filePath);
    }

    if (res.hasErrors()) {
        std::cout << "Compilation exited with errors, unable to proceed.\n";
        return 1;
    }

    // Link shader modules to program
    auto modules = res.shaderStages
        | std::views::transform([](auto& pair){
            return std::make_pair(cloth::parser::toVulkanEnum(pair.first), pair.second.shaderModule);
        })
        | std::ranges::to<std::unordered_map>();

    auto program = trc::shader::linkShaderProgram(modules);
    if (!program)
    {
        std::cout << "Error during shader program linking.\n";
        return 1;
    }
    else {
        std::cout << "Successfully linked shader program.\n";
    }

    auto [tmpl, rp] = trc::PipelineRegistry::cloneGraphicsPipeline(
        trc::pipelines::getDrawableBasePipeline(trc::pipelines::DrawableBasePipelineTypeFlags{})
    );
    auto runtime = trc::makeMaterialProgram(program.value(), tmpl.getPipelineData(), rp);
    if (!runtime)
    {
        std::cout << runtime.error().what() << "\n";
        return 1;
    }
    else {
        std::cout << "Successfully created shader program runtime.\n";
    }

    return 0;
}
