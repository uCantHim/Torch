#include "cloth.h"

#include <algorithm>
#include <iostream>
#include <optional>
#include <ranges>

#include <shader_tools/ShaderDocument.h>
#include <trc/base/Logging.h>
#include <trc/material/shader/DefaultResourceResolver.h>
#include <trc/material/shader/ShaderCodeCompiler.h>
#include <trc/material/shader/ShaderModuleCompiler.h>
#include <trc/material/shader/ShaderResourceInterface.h>
#include <trc/util/TorchDirectories.h>
#include <trc_util/StringManip.h>
#include <trc_util/algorithm/VectorTransform.h>

#include "DocumentUtil.h"
#include "document.h"
#include "parser.h"



namespace cloth
{

void ShaderOutputImpl::defineParameter(const std::string& name, trc::shader::BasicType type)
{
    params.try_emplace(name, type);
}

auto ShaderOutputImpl::getParameters() const
    -> std::generator<std::pair<std::string_view, trc::shader::BasicType>>
{
    co_yield std::ranges::elements_of(params);
}

auto ShaderOutputImpl::getParameterType(const std::string& param) const
    -> std::optional<trc::shader::BasicType>
{
    auto it = params.find(param);
    if (it != params.end()) {
        return it->second;
    }
    return std::nullopt;
}

void ShaderOutputImpl::setParameter(const std::string& param, trc::shader::code::Value value)
{
    paramValues[param] = value;
}

auto ShaderOutputImpl::getParamValues() const
    -> std::generator<std::pair<std::string_view, trc::shader::code::Value>>
{
    co_yield std::ranges::elements_of(paramValues);
}



struct VariableInfo
{
    std::string name;
    FullId varName;

    std::string shaderId;  // Identifier in shader code that represents the value
    std::string declCode;

    std::vector<parser::Location> orderedOccurrences;
    parser::Location firstLoc;  // Location of the variable's first occurrence
    parser::Location lastLoc;   // Location of the variable's last occurrence
};

auto compileShader(
    std::istream& is,
    trc::shader::CapabilityConfig& caps,
    ShaderOutputImpl& outputConfig)
    -> CompileResult
{
    Document doc{ parser::parseClothDocument(is).value() };

    trc::shader::ShaderResourceInterfaceBuilder resources{ caps, caps.getCodeBuilder() };
    trc::shader::CapabilityConfigResourceResolver resolver{ resources };
    trc::shader::ShaderValueCompiler valueCompiler{ resolver, false };
    trc::shader::ShaderModuleBuilder moduleBuilder;

    std::vector<VariableInfo> capabilityVariables;
    std::vector<VariableInfo> outputVariables;

    // Process variables in the shader document
    for (const auto& varName : doc.allVariables())
    {
        auto split = trc::util::splitString(varName.id, ':');
        if (split.size() != 2) {
            continue;
        }

        const auto locs = doc.findOccurrences(varName)
            | std::ranges::to<std::vector>();
        VariableInfo var{
            .name=split[1],
            .varName=varName,
            .shaderId{},   // Will be generated later
            .declCode{},   // Will be generated later
            .orderedOccurrences=locs,
            .firstLoc=locs.front(),
            .lastLoc=locs.back(),
        };

        // Categorize variables by type
        if (split[0] == "cap")
        {
            const trc::shader::Capability cap{ var.name };
            const bool hasCap = caps.hasCapability(cap);
            if (hasCap)
            {
                auto access = resources.queryCapability(cap);
                auto [id, decl] = valueCompiler.compile(access);
                doc.set(var.varName, id);

                var.shaderId = std::move(id);
                var.declCode = std::move(decl);
                capabilityVariables.emplace_back(std::move(var));
            }
            else {
                trc::log::error << "Capability \"" << cap.getName()
                                << "\" is not defined by the shader implementation.\n";
            }
        }
        else if (split[0] == "out")
        {
            const auto type = outputConfig.getParameterType(var.name);
            if (type)
            {
                auto value = moduleBuilder.makeConstant(*type);
                auto [id, decl] = valueCompiler.compile(value);
                doc.set(var.varName, id);

                var.shaderId = std::move(id);
                var.declCode = std::move(decl);
                outputVariables.emplace_back(std::move(var));
            }
            else {
                trc::log::error << "Output parameter \"" << var.name
                                << "\" is not defined by the shader implementation.\n";
            }
        }
        else {
            trc::log::warn << "Variable type \"" << split[0] << "\" not recognized. Skipping.\n";
        }
    }

    auto lines = doc.compile(true).value()
                 | std::views::split('\n')
                 | std::ranges::to<std::vector<std::string>>();

    // Sort variables by first occurrence
    auto allVars = trc::util::merged(capabilityVariables, outputVariables);
    std::ranges::sort(allVars, [](auto& a, auto& b){ return a.firstLoc.line < b.firstLoc.line; });

    // Insert declaration code for variables (both $cap and $out variables)
    // before the variable's first occurrence.
    for (size_t insertedLines = 0; auto& var : allVars)
    {
        auto& declCode = var.declCode;
        const auto lineIdx = var.firstLoc.line + insertedLines;
        if (!declCode.empty())
        {
            // Make the output a bit nicer
            if (declCode.ends_with('\n')) {
                declCode.pop_back();
            }
            if (auto indent = lines[lineIdx].find_first_not_of(' '); indent != std::string::npos) {
                declCode.insert(0, indent, ' ');
            }

            // Insert declaration code for the output variable
            lines.insert(lines.begin() + lineIdx, declCode);
            ++insertedLines;
        }
    }

    // Generate output parameter implementations
    for (const auto& var : outputVariables)
    {
        // Only process output parameters
        auto id = moduleBuilder.makeExternalIdentifier(var.shaderId);
        outputConfig.setParameter(var.name, id);
    }
    auto outputInterface = outputConfig.buildShaderOutputs(moduleBuilder);

    // Create output statements
    trc::shader::code::Block block = std::make_shared<trc::shader::code::BlockT>();
    moduleBuilder.startBlock(block);
    outputInterface.buildStatements(moduleBuilder);
    moduleBuilder.endBlock();
    auto outputStatements = trc::shader::ShaderBlockCompiler{ valueCompiler }.compile(block);

    // Generate shader module code
    spirv::FileIncluder fileIncluder{ {trc::util::getInternalShaderStorageDirectory()} };
    auto typeDecls = moduleBuilder.compileTypeDecls();
    auto functionDecls = moduleBuilder.compileFunctionDecls(resolver);
    auto includedCode = moduleBuilder.compileIncludedCode(fileIncluder, resolver);
    auto shaderResources = resources.compile();

    std::stringstream code;
    code << moduleBuilder.compileSettings();
    code << typeDecls;
    code << shaderResources.getGlslCode();
    code << moduleBuilder.compileOutputLocations();
    code << includedCode;
    code << functionDecls;

    // Insert output statements at the end of main
    if (const auto main = util::findMain(lines))
    {
        lines[main->bodyEnd.line].insert(main->bodyEnd.pos, outputStatements);
        lines[main->bodyEnd.line].insert(main->bodyEnd.pos, "\n// Cloth-generated output statements:");
    }
    else {
        lines.emplace_back("\n// Cloth-generated main function:");
        lines.emplace_back("void main() {");
        lines.emplace_back(outputStatements);
        lines.emplace_back("}");
    }

    // Append original Cloth shader code (modified)
    code << std::ranges::to<std::string>(std::views::join_with(lines, '\n'));

    return CompileResult{
        .shaderModule{ shader_edit::ShaderDocument{ code.str() }, shaderResources },
    };
}

} // namespace cloth
