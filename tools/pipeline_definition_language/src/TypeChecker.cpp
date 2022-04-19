#include "TypeChecker.h"

#include <cassert>

#include "Exceptions.h"
#include "ErrorReporter.h"



template<typename... Ts> struct VariantVisitor : Ts... { using Ts::operator()...; };
template<typename... Ts> VariantVisitor(Ts...) -> VariantVisitor<Ts...>;

auto makeDefaultTypeConfig() -> TypeConfiguration
{
    return TypeConfiguration({
        // Fundamental string type
        { stringTypeName, StringType{} },

        // Type of the "global" object, which has both arbitrary field names
        // and some pre-defined fields (e.g. 'Meta')
        { globalObjectTypeName, ObjectType{
            .typeName=globalObjectTypeName,
            .fields={
                { "Meta", { "CompilerMetaData" } },
            }
        }},
        { "CompilerMetaData", ObjectType{
            .typeName="CompilerMetaData",
            .fields={
                { "BaseDir", { stringTypeName } },
            }
        }},

        // The following are custom (non-built-in) types
        { "Variable", StringType{} },
        { "Shader", ObjectType{
            .typeName="Shader",
            .fields={
                { "Source",   { stringTypeName } },
                { "Variable", { "Variable", FieldType::eMap } },
            }
        }},
        { "Program", ObjectType{
            .typeName="Program",
            .fields={
                { "VertexShader", { "Shader" } },
            }
        }},
        { "Pipeline", ObjectType{
            .typeName="Pipeline",
            .fields={
                { "Base",    { "Pipeline" } },
                { "Program", { "Program" } },
            }
        }}
    });
}



TypeChecker::TypeChecker(TypeConfiguration config, ErrorReporter& errorReporter)
    :
    config(std::move(config)),
    errorReporter(&errorReporter)
{
}

bool TypeChecker::check(const std::vector<Stmt>& statements)
{
    bool hadError{ false };
    for (const auto& stmt : statements)
    {
        try {
            std::visit(*this, stmt);
        }
        catch (const TypeError& err)
        {
            hadError = true;
            errorReporter->error({
                .location=err.token.location,
                .message="At token " + to_string(err.token.type) + ": " + err.message
            });
        }
    }

    return hadError;
}

void TypeChecker::operator()(const TypeDef& def)
{
    std::visit(*this, def);
}

void TypeChecker::operator()(const EnumTypeDef& def)
{
    auto [it, success] = config.types.try_emplace(def.name, EnumType{ def.name, def.options });
    if (!success) {
        error({}, "Duplicate type definition.");
    }
}

void TypeChecker::operator()(const FieldDefinition& def)
{
    const auto& globalObj = std::get<ObjectType>(this->config.types.at(globalObjectTypeName));
    checkFieldDefinition(globalObj, def);

    // Statement is valid, add defined variable to lookup table
    if (std::holds_alternative<TypedFieldName>(def.name))
    {
        const auto& field = std::get<TypedFieldName>(def.name);
        identifierValues.try_emplace(field.name, def.value.get());
    }
}

void TypeChecker::checkFieldDefinition(const ObjectType& parent, const FieldDefinition& def)
{
    TypeName expected = std::visit(VariantVisitor{
        [this](const TypedFieldName& name){ return name.type.name; },
        [this, &parent](const TypelessFieldName& name){
            if (!parent.fields.contains(name.name.name)) {
                error(name.name.token, "Invalid field name \"" + name.name.name + "\".");
            }
            return parent.fields.at(name.name.name).storedType;
        },
    }, def.name);

    if (!config.types.contains(expected)) {
        error(getToken(*def.value), "Expected undefined type \"" + expected + "\".");
    }

    if (!std::visit(CheckValueType{ config.types.at(expected), *this }, *def.value)) {
        error({}, "Encountered value of unexpected type.");
    }
}

auto TypeChecker::getToken(const FieldValue& val) -> const Token&
{
    return std::visit([](const auto& v) -> const Token& { return v.token; }, val);
}

void TypeChecker::error(const Token& token, std::string message)
{
    throw TypeError{
        token,
        std::move(message)
    };
}



TypeChecker::CheckValueType::CheckValueType(TypeType& expected, TypeChecker& self)
    :
    expectedType(&expected),
    expectedTypeName(std::visit([](auto& a){ return a.typeName; }, expected)),
    self(&self)
{
}

bool TypeChecker::CheckValueType::operator()(const LiteralValue& val) const
{
    if (!std::holds_alternative<StringType>(*expectedType)) {
        expectedTypeError(val.token, StringType::typeName);
    }
    return true;
}

bool TypeChecker::CheckValueType::operator()(const Identifier& id) const
{
    // Find identifier's type in lookup table
    auto it = self->identifierValues.find(id);
    if (it == self->identifierValues.end()) {
        self->error(id.token, "Use of undeclared identifier \"" + id.name + "\"");
    }

    // Check the referenced value
    return std::visit(CheckValueType{ *this }, *it->second);
}

bool TypeChecker::CheckValueType::operator()(const ObjectDeclaration& obj) const
{
    if (!std::holds_alternative<ObjectType>(*expectedType)) {
        expectedTypeError(obj.token, undefinedObjectType);
    }

    auto& objectType = std::get<ObjectType>(*expectedType);
    for (const auto& field : obj.fields)
    {
        try {
            self->checkFieldDefinition(objectType, field);
        }
        catch (const TypeError& err) {
            // Replace generated error with the correct location
            self->error(err.token, err.message);
        }
    }

    return true;
}

bool TypeChecker::CheckValueType::operator()(const MatchExpression& expr) const
{
    for (const auto& _case : expr.cases)
    {
        try {
            if (!std::visit(CheckValueType{ *this }, *_case.value)) {
                return false;
            }
        }
        catch (const TypeError& err) {
            // Replace generated error with the correct location
            self->error(getToken(*_case.value), err.message);
        }
    }

    return true;
}

void TypeChecker::CheckValueType::expectedTypeError(
    const Token& token,
    const TypeName& actualType) const
{
    self->error(token, "Expected value of type \"" + expectedTypeName + "\""
                       ", but got \"" + actualType + "\".");
}
