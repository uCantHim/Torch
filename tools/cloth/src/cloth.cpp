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
#include "builtins.h"
#include "document.h"
#include "parser.h"



namespace cloth
{

auto compileShader(
    std::istream& is,
    trc::shader::CapabilityConfig& caps,
    ShaderOutputImpl& outputConfig)
    -> std::expected<CompileResult, CompileError>
{
    auto parseResult = parser::parseDocument(is);
    if (!parseResult)
    {
        auto& err = parseResult.error();
        return std::unexpected(CompileError{
            .errors=std::move(err.errors),
            .initialDocumentLines=std::move(err.partialResult.lines),
        });
    }

    Document doc{ parseResult.value() };

    trc::shader::ShaderResourceInterfaceBuilder resources{ caps, caps.getCodeBuilder() };
    trc::shader::CapabilityConfigResourceResolver resolver{ resources };
    trc::shader::ShaderValueCompiler valueCompiler{ resolver, false };
    trc::shader::ShaderModuleBuilder moduleBuilder;

    std::vector<FullId> outputVariables;
    std::unordered_map<FullId, trc::shader::code::Value> varImpls;

    // Process variables in the shader document
    BuiltinProvider clothImpl;
    for (const auto& varId : doc.allVariables())
    {
        auto value = clothImpl.makeValue(varId, {}, moduleBuilder);
        if (value)
        {
            varImpls.try_emplace(varId, value.value());
            if (varId.nsQualifier && varId.nsQualifier == "out") {
                outputVariables.emplace_back(varId);
            }
        }
        else {
            trc::log::warn << "Cloth built-in \"" << varId.id << "\" not defined: "
                           << value.error().message << ". Skipping.";
        }
    }

    // Generate code for Cloth built-ins.
    std::unordered_map<FullId, std::string> shaderId;
    std::unordered_map<FullId, std::string> declCode;
    for (const auto& [var, value] : varImpls)
    {
        // Generate code for the variable's value.
        auto [id, decl] = valueCompiler.compile(value);
        doc.set(var, id);
        shaderId.try_emplace(var, std::move(id));
        declCode.try_emplace(var, std::move(decl));
    }

    // Create document version with all variables set.
    auto lines = doc.compile(false).value()
                 | std::views::split('\n')
                 | std::ranges::to<std::vector<std::string>>();

    // Sort variables by first occurrence
    auto firstOccurrences = doc.allVariables()
        | std::views::transform([&doc](auto&& id) {
            auto firstOcc = *doc.findOccurrences(id).begin();
            return std::make_pair(id, firstOcc);
        })
        | std::ranges::to<std::vector>();
    std::ranges::sort(firstOccurrences, [](auto& a, auto& b){ return a.second.line < b.second.line; });

    // Insert declaration code for variables (both $cap and $out variables)
    // before the variable's first occurrence.
    for (size_t insertedLines = 0; const auto& [var, loc] : firstOccurrences)
    {
        auto& code = declCode.at(var);

        // Insert declaration code before the variable's first occurrence.
        const auto lineIdx = loc.line + insertedLines;
        if (!code.empty())
        {
            // Make the output a bit nicer
            if (code.ends_with('\n')) {
                code.pop_back();
            }
            if (auto indent = lines[lineIdx].find_first_not_of(' '); indent != std::string::npos) {
                code.insert(0, indent, ' ');
            }

            // Insert declaration code for the output variable
            lines.insert(lines.begin() + lineIdx, code);
            ++insertedLines;
        }
    }

    // Generate output parameter implementations
    for (const auto& var : outputVariables)
    {
        auto id = moduleBuilder.makeExternalIdentifier(shaderId.at(var));
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
        lines[main->bodyEnd.line].insert(main->bodyEnd.pos, "\n// Cloth-generated output statements:\n");
    }
    else {
        lines.emplace_back("\n// Cloth-generated main function:");
        lines.emplace_back("void main() {");
        lines.emplace_back(outputStatements);
        lines.emplace_back("}");
    }

    // Append original Cloth shader code (modified)
    code << std::ranges::to<std::string>(std::views::join_with(lines, '\n'));

    // Debug:
    //std::cout << "\nFinal shader code:\n" << code.str() << "\n";

    /*
     * Note: Because we only output a single shader module (not a full shader
     * program), the final code generated here is likely to contain unset
     * variables, such as descriptor index placeholders. These will be set
     * during shader program linking.
     */
    return CompileResult{
        .shaderModule{ shader_edit::ShaderDocument{ code.str() }, std::move(shaderResources) },
    };
}

} // namespace cloth
