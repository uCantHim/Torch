#include "cloth.h"

#include <algorithm>
#include <iostream>
#include <optional>
#include <ranges>
#include <utility>

#include <shader_tools/ShaderDocument.h>
#include <trc/base/Logging.h>
#include <trc/material/shader/DefaultResourceResolver.h>
#include <trc/material/shader/ShaderCodeCompiler.h>
#include <trc/material/shader/ShaderModuleCompiler.h>
#include <trc/material/shader/ShaderResourceInterface.h>
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

auto compileShader(
    std::istream& is,
    const trc::shader::CapabilityConfig& caps,
    std::unique_ptr<ShaderOutputImpl> outputConfig)
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
    auto compileResult = compileShader(parsed, caps, std::move(outputConfig));
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

auto compileShader(const parser::Result& parseResult,
                   const trc::shader::CapabilityConfig& caps,
                   std::unique_ptr<ShaderOutputImpl> outputConfig)
    -> std::expected<CompileResult, CompileError>
{
    assert(outputConfig);

    Document doc{ parseResult };
    trc::shader::ShaderModuleBuilder moduleBuilder;

    // Generate values for Cloth built-ins.
    BuiltinProvider clothImpl;
    auto builtinCompileResult = compileBuiltins(doc, clothImpl, moduleBuilder);
    if (!builtinCompileResult) {
        return std::unexpected(builtinCompileResult.error());
    }
    auto& [outputVariables, varImpls] = *builtinCompileResult;

    // Generate and insert code into document
    trc::shader::ShaderResourceInterfaceBuilder resources{ caps, moduleBuilder };
    trc::shader::CapabilityConfigResourceResolver resolver{ resources };
    trc::shader::ShaderValueCompiler valueCompiler{ resolver, false };

    std::unordered_map<parser::Location, std::string> shaderId;
    std::vector<std::pair<parser::Location, std::string>> declCode;
    for (const auto& [var, value] : varImpls)
    {
        // Generate code for the variable's value.
        auto [id, decl] = valueCompiler.compile(value);
        doc.set(*var, id);
        shaderId.try_emplace(var->location, std::move(id));
        declCode.emplace_back(var->location, std::move(decl));
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
    for (const auto& var : outputVariables)
    {
        auto id = moduleBuilder.makeExternalIdentifier(shaderId.at(var->location));
        outputConfig->setParameter(var->id.name, id);
    }
    auto outputInterface = outputConfig->buildShaderOutputs(moduleBuilder);

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
    // std::cout << "\nFinal shader code:\n" << code.str() << "\n";

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

auto printErrors(const cloth::CompileError& doc, std::optional<std::string> filePath)
    -> std::string
{
    auto indent = [](size_t n, char c = ' ') { return std::string(n, c); };

    std::stringstream ss;
    ss << "Compile error.\n";
    for (const auto& err : doc.errors)
    {
        const auto& lines = doc.initialDocumentLines;
        const auto loc = err.location;

        // Error message
        if (filePath) {
            ss << *filePath << ":";
        }
        ss << loc.line << ":" << loc.firstChar << ": error: " << err.message << "\n";
        // Code line
        ss << "  " << loc.line << " | " << lines.at(loc.line) << "\n";
        // Positional indicator line
        ss << "  " << indent(std::to_string(loc.line).size())
                  << " | " << indent(loc.firstChar)
                  << "^" << indent(loc.endChar - loc.firstChar - 1, '~') << "\n";
    }

    return ss.str();
}

} // namespace cloth
