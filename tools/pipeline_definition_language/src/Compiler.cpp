#include "Compiler.h"

#include <iostream>

#include "Exceptions.h"
#include "Util.h"



Compiler::Compiler(std::vector<Stmt> _statements, ErrorReporter& errorReporter)
    :
    converter(std::move(_statements)),
    errorReporter(&errorReporter)
{
}

auto Compiler::compile() -> CompileResult
{
    auto globalObject = converter.convert();

    auto it = globalObject.fields.find("Pipeline");
    if (it != globalObject.fields.end()) {
        compilePipelines(expectMap(it->second));
    }
    it = globalObject.fields.find("Shader");
    if (it != globalObject.fields.end()) {
        compileShaders(expectMap(it->second));
    }

    result.flagTable = converter.getFlagTable();
    return result;
}

void Compiler::error(const Token& token, std::string message)
{
    errorReporter->error(Error{ .location=token.location, .message=std::move(message) });
}

auto Compiler::expectField(const compiler::Object& obj, const std::string& field)
    -> const compiler::FieldValueType&
{
    try {
        return obj.fields.at(field);
    }
    catch (const std::out_of_range& err) {
        throw InternalLogicError(err.what());
    }
}

auto Compiler::expectSingle(const compiler::FieldValueType& field) -> const compiler::Value&
{
    return *std::get<compiler::SingleValue>(field).value;
}

auto Compiler::expectMap(const compiler::FieldValueType& field) -> const compiler::MapValue&
{
    return std::get<compiler::MapValue>(field);
}

auto Compiler::expectLiteral(const compiler::Value& val) -> const compiler::Literal&
{
    return std::get<compiler::Literal>(val);
}

auto Compiler::expectObject(const compiler::Value& val) -> const compiler::Object&
{
    return std::get<compiler::Object>(val);
}

void Compiler::compileShaders(const compiler::MapValue& val)
{
    for (const auto& [name, value] : val.values)
    {
        if (std::holds_alternative<compiler::Variated>(*value))
        {
            VariantGroup<ShaderDesc> shaders{ .baseName=name };
            for (const auto& var : std::get<compiler::Variated>(*value).variants)
            {
                auto shader = compileShader(expectObject(*var.value));
                shaders.variants.try_emplace(UniqueName(name, var.setFlags), std::move(shader));
            }
            if (shaders.variants.empty()) {
                throw InternalLogicError("Variated value must have at least one variant");
            }
            // Collect all flag types
            for (auto& flag : shaders.variants.begin()->first.getFlags()) {
                shaders.flagTypes.emplace_back(flag.flagId);
            }
            result.shaders.try_emplace(name, std::move(shaders));
        }
        else {
            auto shader = compileShader(expectObject(*value));
            result.shaders.try_emplace(name, std::move(shader));
        }
    }
}

auto Compiler::compileShader(const compiler::Object& obj) -> ShaderDesc
{
    ShaderDesc res;

    res.source = expectLiteral(expectSingle(expectField(obj, "Source"))).value;
    auto it = obj.fields.find("Variable");
    if (it != obj.fields.end())
    {
        for (const auto& [name, val] : expectMap(it->second).values)
        {
            res.variables.try_emplace(name, expectLiteral(*val).value);
        }
    }

    return res;
}

void Compiler::compilePipelines(const compiler::MapValue& val)
{
    for (const auto& [name, value] : val.values)
    {
        if (std::holds_alternative<compiler::Variated>(*value))
        {
            for (const auto& var : std::get<compiler::Variated>(*value).variants)
            {
                auto pipeline = compilePipeline(expectObject(*var.value));
            }
        }
        else {
            auto pipeline = compilePipeline(expectObject(*value));
        }
    }
}

auto Compiler::compilePipeline(const compiler::Object& obj) -> PipelineDesc
{
    return {};
}
