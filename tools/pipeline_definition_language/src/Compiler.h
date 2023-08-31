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
    bool hasField(const compiler::Object& obj, const std::string& field);
    auto expectField(const compiler::Object& obj, const std::string& field)
        -> const compiler::FieldValueType&;
    auto expectSingle(const compiler::FieldValueType& field) -> const compiler::Value&;
    auto expectMap(const compiler::FieldValueType& field) -> const compiler::MapValue&;
    auto expectSingle(const compiler::Object& obj, const std::string& field)
        -> const compiler::Value&;
    auto expectMap(const compiler::Object& obj, const std::string& field)
        -> const compiler::MapValue&;

    template<typename T>
    auto expect(const compiler::Value& val) -> const T&;
    auto expectLiteral(const compiler::Value& val) -> const compiler::Literal&;
    auto expectList(const compiler::Value& val) -> const compiler::List&;
    auto expectObject(const compiler::Value& val) -> const compiler::Object&;

    template<typename T>
    auto expect(const compiler::Literal& val) -> const T&;
    auto expectString(const compiler::Value& val) -> const std::string&;
    auto expectFloat(const compiler::Value& val) -> double;
    auto expectInt(const compiler::Value& val) -> int64_t;
    auto expectBool(const compiler::Value& val) -> bool;

    /**
     * Creates a reference from either an inline object or an actual
     * reference.
     */
    template<typename T>
    auto makeReference(const compiler::Value& val) -> ObjectReference<T>;

    /** Create a reference in a field is present, or nothing otherwise. */
    template<typename T>
    auto makeOptReference(const compiler::Object& obj, const std::string& field)
        -> std::optional<ObjectReference<T>>;

    auto compileMeta(const compiler::Object& metaObj) -> CompileResult::Meta;

    template<typename T>
    auto compileMulti(const compiler::MapValue& val)
        -> std::unordered_map<std::string, CompileResult::SingleOrVariant<T>>;

    /**
     * Compile a typeless object definition into a simplified object type.
     *
     * Every object defined anywhere in the compiled source (also those defined
     * as an inline object reference or variated object collection) passes
     * through this function at some point.
     */
    template<typename T>
    auto compileSingle(const compiler::Object& obj) -> T;

    compiler::ObjectConverter converter;
    CompileResult result;

    compiler::Object globalObject;
    std::unordered_map<std::string, std::shared_ptr<compiler::Value>> referenceTable;
    const VariantFlagSet* currentVariant{ nullptr };

    ErrorReporter* errorReporter;
};
