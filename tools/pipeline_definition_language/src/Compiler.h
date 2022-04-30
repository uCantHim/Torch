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
#include "ObjectConverter.h"

class ErrorReporter;

class Compiler
{
public:
    Compiler(std::vector<Stmt> statements, ErrorReporter& errorReporter);

    auto compile() -> CompileResult;

private:
    static bool isSubset(const VariantFlagSet& set, const VariantFlagSet& subset);
    static auto findVariant(const compiler::Variated& variated, const VariantFlagSet& flags)
        -> const compiler::Variated::Variant*;
    static auto makeReferenceName() -> std::string;

    void error(const Token& token, std::string message);

    auto resolveReference(const compiler::Reference& ref) -> const compiler::Value&;
    auto expectField(const compiler::Object& obj, const std::string& field)
        -> const compiler::FieldValueType&;
    auto expectSingle(const compiler::FieldValueType& field) -> const compiler::Value&;
    auto expectMap(const compiler::FieldValueType& field) -> const compiler::MapValue&;

    template<typename T>
    auto expect(const compiler::Value& val) -> const T&;
    auto expectLiteral(const compiler::Value& val) -> const compiler::Literal&;
    auto expectList(const compiler::Value& val) -> const compiler::List&;
    auto expectObject(const compiler::Value& val) -> const compiler::Object&;

    /**
     * Creates a reference from either an inline object or an actual
     * reference.
     */
    template<typename T>
    auto makeReference(const compiler::Value& val) -> ObjectReference<T>;

    template<typename T>
    auto compileMulti(const compiler::MapValue& val)
        -> std::unordered_map<std::string, CompileResult::SingleOrVariant<T>>;
    template<typename T>
    auto compileSingle(const compiler::Object& obj) -> T;

    compiler::ObjectConverter converter;
    CompileResult result;

    compiler::Object globalObject;
    std::unordered_map<std::string, std::shared_ptr<compiler::Value>> referenceTable;
    const VariantFlagSet* currentVariant{ nullptr };

    ErrorReporter* errorReporter;
};
