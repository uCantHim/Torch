#include "cloth.h"

#include <algorithm>
#include <iostream>
#include <optional>
#include <ranges>
#include <utility>

#include <shader_tools/ShaderDocument.h>
#include <trc/base/Logging.h>
#include <trc/material/shader/ShaderCodeCompiler.h>
#include <trc/material/shader/ShaderModuleCompiler.h>
#include <trc/material/shader/ShaderResourceInterface.h>
#include <trc/material/shader/ShaderStageInputLinker.h>
#include <trc/util/TorchDirectories.h>
#include <trc_util/StringManip.h>
#include <trc_util/algorithm/VectorTransform.h>

#include "parse_utils.h"
#include "builtins.h"
#include "document.h"
#include "parser.h"



namespace cloth
{

struct BuiltinCompilationResult
{
    std::vector<const parser::Variable*> outputVariables;
    std::unordered_map<const parser::Variable*, trc::shader::code::Value> varImpls;
};

/**
 * Compile a variable to a shader code expression.
 *
 * This pushes the generated code into the respective arrays `varImpls` and
 * `outputVariables` as a side effect because it calls itself recursively
 * for the variable's arguments.
 */
auto compileVariable(
    const parser::Variable& var,
    BuiltinProvider& clothImpl,
    trc::shader::ShaderModuleBuilder& moduleBuilder)
    -> std::expected<trc::shader::code::Value, CompileError>
{
    auto error = [](parser::Error&& err) {
        return std::unexpected(CompileError{ .errors{ std::move(err) }, .initialDocumentLines{} });
    };

    // Perform some upfront validation on the argument list's shape.
    auto builtin = clothImpl.getDefinition(var.id);
    if (builtin == nullptr)
    {
        return error({
            .location=var.location,
            .message=std::format("\"{}\" is not a Cloth built-in.", var.id.id),
        });
    }

    if (var.arguments.size() != builtin->args.size())
    {
        return error({
            .location=var.location,
            .message=std::format("\"{}\" expects {} argument(s), but got {}.",
                                 builtin->fullId,
                                 builtin->args.size(),
                                 var.arguments.size()),
        });
    }

    // Compile arguments to values.
    auto compileArg = [&](const parser::Argument& arg)
        -> std::expected<Builtin::ArgValue, CompileError>
    {
        switch (arg.type)
        {
        case parser::Argument::Type::eVariable:
            return compileVariable(std::any_cast<const parser::Variable&>(arg.content),
                                   clothImpl,
                                   moduleBuilder)
                .transform([](auto val){ return Builtin::ArgValue{ val }; });
        case parser::Argument::Type::eString:
            return trc::AssetPath{ std::any_cast<std::string>(arg.content) };
        case parser::Argument::Type::eExternalExpression:
            return moduleBuilder.makeExternalIdentifier(std::any_cast<std::string>(arg.content));
        }
        std::unreachable();
    };

    std::vector<Builtin::ArgValue> argValues;
    for (const auto& arg : var.arguments)
    {
        auto value = compileArg(arg);
        if (value) {
            argValues.emplace_back(std::move(*value));
        }
        else {
            return std::unexpected(value.error());
        }
    }

    // Create final value.
    auto value = clothImpl.makeValue(var.id, argValues, moduleBuilder);
    if (value) {
        return *value;
    }
    return error(parser::Error{
        var.location,
        std::format("Unable to process item \"{}\": {}", var.id.id, value.error().message)
    });
};

auto compileBuiltins(
    const Document& doc,
    BuiltinProvider& clothImpl,
    trc::shader::ShaderModuleBuilder& moduleBuilder)
    -> std::expected<BuiltinCompilationResult, CompileError>
{
    std::vector<parser::Error> errors;

    std::vector<const parser::Variable*> outputVariables;
    std::unordered_map<const parser::Variable*, trc::shader::code::Value> varImpls;

    for (const auto& var : doc.allVariables())
    {
        auto value = compileVariable(var, clothImpl, moduleBuilder);
        if (value)
        {
            varImpls.try_emplace(&var, value.value());
            if (var.id.nsQualifier && var.id.nsQualifier == "out") {
                outputVariables.emplace_back(&var);
            }
        }
        else {
            trc::util::merge(errors, value.error().errors);
        }
    }

    if (!errors.empty()) {
        return std::unexpected(CompileError{ std::move(errors), doc.getLines() });
    }
    return BuiltinCompilationResult{
        .outputVariables=std::move(outputVariables),
        .varImpls=std::move(varImpls),
    };
}

struct PartialResult
{
    std::vector<std::string> clothLines;
    trc::shader::ShaderModuleBuilder builder;
    trc::shader::ShaderOutputInterface outputs;
    trc::shader::ShaderValueCompiler valueCompiler;
};

auto compilePartial(const parser::Result& parseResult, BackendConfig& impl)
    -> std::expected<PartialResult, CompileError>
{
    auto makeError = [&](parser::Location loc, std::string msg) {
        return std::unexpected(CompileError{
            .errors{ parser::Error{ loc, std::move(msg) } },
            .initialDocumentLines=parseResult.lines,
        });
    };

    Document doc{ parseResult };
    trc::shader::ShaderModuleBuilder moduleBuilder{ impl.makeCapabilityConfig() };

    // Generate values for Cloth built-ins.
    auto builtinCompileResult = compileBuiltins(doc, impl.getBuiltins(), moduleBuilder);
    if (!builtinCompileResult) {
        return std::unexpected(builtinCompileResult.error());
    }
    auto& [outputVariables, varImpls] = *builtinCompileResult;

    // Generate code from the values of cloth built-ins and set document
    // variables.
    trc::shader::ShaderValueCompiler valueCompiler{ false };

    std::unordered_map<parser::Location, std::string> shaderId;
    std::vector<std::pair<parser::Location, std::string>> declCode;
    for (const auto& [var, value] : varImpls)
    {
        try {
            // Generate code for the variable's value.
            auto [id, decl] = valueCompiler.compile(value);
            doc.set(*var, id);
            shaderId.try_emplace(var->location, std::move(id));
            declCode.emplace_back(var->location, std::move(decl));
        }
        catch (const std::exception& err) {
            return makeError(var->location,
                             std::format("Unable to generate code for variable \"{}\": {}",
                                         var->id.id, err.what()));
        }
    }

    // Create document version with all variables set.
    auto lines = doc.compile()
                 | std::views::split('\n')
                 | std::ranges::to<std::vector<std::string>>();

    // Sort declaration code by location
    std::ranges::sort(declCode, [](auto& a, auto& b){ return a.first < b.first; });

    // Insert declaration code for variables (both $cap and $out variables)
    // before the variable's first occurrence.
    for (size_t insertedLines = 0; auto& [loc, code] : declCode)
    {
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
    auto outputConfig = impl.makeOutputConfig();
    for (const auto& var : outputVariables)
    {
        auto id = moduleBuilder.makeExternalIdentifier(shaderId.at(var->location));
        auto outParam = impl.outputBuiltinToParameter(var->id);
        if (!outParam)
        {
            return makeError(
                var->location,
                std::format("Cloth built-in \"{}\" is not defined as a shader output.", var->id.id)
            );
        }

        outputConfig->setParameter(*outParam, id);
    }
    auto shaderOutputs = outputConfig->makeOutputs(moduleBuilder);

    // Create result
    return PartialResult{
        .clothLines = std::move(lines),
        .builder = std::move(moduleBuilder),
        .outputs = std::move(shaderOutputs),
        .valueCompiler = std::move(valueCompiler),
    };
}

auto generateCode(PartialResult& partial) -> std::string
{
    auto& moduleBuilder = partial.builder;
    auto& shaderOutputs = partial.outputs;
    auto& valueCompiler = partial.valueCompiler;
    auto& clothLines = partial.clothLines;

    // Create output statements
    trc::shader::code::Block block = std::make_shared<trc::shader::code::BlockT>();
    moduleBuilder.startBlock(block);
    shaderOutputs.buildStatements(moduleBuilder);
    moduleBuilder.endBlock();
    auto outputStatements = trc::shader::ShaderBlockCompiler{ valueCompiler }.compile(block);

    // Generate shader module code
    spirv::FileIncluder fileIncluder{ {trc::util::getInternalShaderStorageDirectory()} };

    std::stringstream code;
    code << moduleBuilder.compileSettings();
    code << moduleBuilder.compileTypeDecls();
    code << moduleBuilder.compileInputResources();
    code << moduleBuilder.compileOutputLocations();
    code << moduleBuilder.compileIncludedCode(fileIncluder);
    code << moduleBuilder.compileFunctionDecls();

    // Insert output statements at the end of main
    if (const auto main = util::findMain(clothLines))
    {
        clothLines[main->bodyEnd.line].insert(main->bodyEnd.pos, outputStatements);
        clothLines[main->bodyEnd.line].insert(main->bodyEnd.pos,
                                         "\n// Cloth-generated output statements:\n");
    }
    else {
        clothLines.insert(clothLines.begin(), "\n// Cloth-generated main function:");
        clothLines.insert(clothLines.begin(), "void main() {");
        clothLines.insert(clothLines.end(), outputStatements);
        clothLines.insert(clothLines.end(), "}");
    }

    code << std::ranges::to<std::string>(std::views::join_with(clothLines, '\n'));

    // Debug:
    //std::cout << "\nFinal shader code:\n" << code.str() << "\n";

    /**
     * Note: Because we only output a single shader module (not a full shader
     * program), the final code generated here is likely to contain unset
     * variables, such as descriptor index placeholders. These will be set
     * during shader program linking.
     */
    return code.str();
}

auto compileShader(std::istream& is, BackendConfig& impl)
    -> std::expected<CompileResult, CompileError>
{
    // Return both parse- and compile errors
    std::vector<cloth::parser::Error> errors;

    // Parse document
    auto parseResult = parser::parseDocument(is);
    if (!parseResult) {
        errors = std::move(parseResult.error().errors);
    }

    // Compile document to shader module
    auto parsed = parseResult ? *parseResult : parseResult.error().partialResult;
    auto compileResult = compileShader(parsed, impl);
    if (!compileResult)
    {
        return std::unexpected(CompileError{
            .errors=trc::util::merged(compileResult.error().errors, errors),
            .initialDocumentLines=std::move(parsed.lines),
        });
    }
    if (!parseResult)
    {
        return std::unexpected(CompileError{
            .errors=std::move(errors),
            .initialDocumentLines=std::move(parsed.lines),
        });
    }

    return compileResult.value();
}

auto compileShader(const parser::Result& parseResult, BackendConfig& impl)
    -> std::expected<CompileResult, CompileError>
{
    return compilePartial(parseResult, impl)
        .transform([](PartialResult res){
            return CompileResult{
                .shaderModule = trc::shader::ShaderModule{
                    shader_edit::ShaderDocument{ generateCode(res) },
                    res.builder.getResourceInterface()
                },
            };
        });
}

auto printErrors(const cloth::CompileError& doc, std::optional<std::string> filePath)
    -> std::string
{
    return "Compile error.\n"
           + util::formatErrors(doc.errors, doc.initialDocumentLines, filePath);
}

auto compileMultiShader(
    std::istream& is,
    const std::unordered_map<parser::ShaderStage, std::shared_ptr<BackendConfig>>& impl
    ) -> MultiCompileResult
{
    const auto lines = trc::util::readLines(is);
    return compileMultiShader(parser::parseMultiDocument(lines), impl);
}

auto compileMultiShader(
    const parser::MultiDocumentResult& parsedDocument,
    const std::unordered_map<parser::ShaderStage, std::shared_ptr<BackendConfig>>& impl
    ) -> MultiCompileResult
{
    // Get the location of a block's declaration header.
    auto getBlockDeclLocation = [&](parser::ShaderStage stage) {
        auto loc = parsedDocument.shaderStageLocations.at(stage);
        return parser::Location{
            (parser::ui32)loc.declBegin.line,
            loc.declBegin.pos,
            parsedDocument.originalLines[loc.declBegin.line].size()
        };
    };

    MultiCompileResult result;
    result.errors.append_range(parsedDocument.allErrors());

    std::vector<std::pair<parser::ShaderStage, PartialResult>> partialStages;
    for (const auto& [stage, parseResult] : parsedDocument.shaderStages)
    {
        assert(parsedDocument.shaderStageLocations.contains(stage));
        if (!impl.contains(stage)) {
            result.errors.emplace_back(parser::Error{
                .location = getBlockDeclLocation(stage),
                .message  = "The Cloth backend does not implement this stage."
            });
        }

        auto partial = compilePartial(parseResult, *impl.at(stage));
        if (partial) {
            partialStages.emplace_back(stage, std::move(*partial));
        }
        else {
            // Fix error locations
            const auto& loc = parsedDocument.shaderStageLocations.at(stage);
            for (auto& err : partial.error().errors) {
                err.location.line += loc.bodyBegin.line;
            }

            result.errors.append_range(partial.error().errors);
        }
    }

    // Link shader stage inputs/outputs
    auto stageLinkInfo = partialStages
        | std::views::transform([](auto& pair) {
            PartialResult& partial = pair.second;
            return std::make_pair(
                parser::toVulkanEnum(pair.first),
                trc::shader::ModuleLinkInfo{
                    .builder=&partial.builder,
                    .resources=&partial.builder.getResourceInterface(),
                    .outputs=&partial.outputs
                }
            );
        })
        | std::ranges::to<std::unordered_map>();

    auto linkResult = trc::shader::linkShaderStageInputs(stageLinkInfo);
    if (!linkResult)
    {
        for (const auto& [stage, inputs] : linkResult.error().unresolvedInputs)
        {
            auto loc = parsedDocument.shaderStageLocations.at(parser::fromVulkanEnum(stage));
            parser::Location stageLoc{
                (parser::ui32)loc.declBegin.line,
                loc.declBegin.pos,
                parsedDocument.originalLines[loc.declBegin.line].size()
            };

            for (const auto& input : inputs)
            {
                result.errors.emplace_back(parser::Error{
                    stageLoc,
                    std::format("Stage link error: Requested capability {}"
                                " is not provided by any prior shader stage.",
                                input.capability.getName()),
                });
            }
        }
    }

    // Generate code for the finalized, linked modules
    result.shaderStages = partialStages
        | std::views::transform([](auto& pair) {
            return std::make_pair(pair.first, CompileResult{
                .shaderModule = trc::shader::ShaderModule{
                    shader_edit::ShaderDocument{ generateCode(pair.second) },
                    pair.second.builder.getResourceInterface()
                },
            });
        })
        | std::ranges::to<std::unordered_map>();

    return result;
}

} // namespace cloth
