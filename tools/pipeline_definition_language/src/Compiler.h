#pragma once

#include <vector>
#include <unordered_map>
#include <functional>

#include "SyntaxElements.h"
#include "TypeConfiguration.h"
#include "CompileResult.h"
#include "FlagTable.h"
#include "IdentifierTable.h"
#include "VariantResolver.h"
#include "CompilerObjectRepresentation.h"

class ErrorReporter;

class Compiler
{
public:
    Compiler(std::vector<Stmt> statements, ErrorReporter& errorReporter);

    auto compile() -> CompileResult;

private:
    void error(const Token& token, std::string message);

    auto expectField(const compiler::Object& obj, const std::string& field)
        -> const compiler::FieldValueType&;
    auto expectSingle(const compiler::FieldValueType& field) -> const compiler::Value&;
    auto expectMap(const compiler::FieldValueType& field) -> const compiler::MapValue&;

    auto expectLiteral(const compiler::Value& val) -> const compiler::Literal&;
    auto expectObject(const compiler::Value& val) -> const compiler::Object&;

    void compileShaders(const compiler::MapValue& val);
    auto compileShader(const compiler::Object& obj) -> ShaderDesc;
    void compilePipelines(const compiler::MapValue& val);
    auto compilePipeline(const compiler::Object& obj) -> PipelineDesc;

    compiler::ObjectConverter converter;
    CompileResult result;

    ErrorReporter* errorReporter;
};
