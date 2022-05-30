#include "TypeChecker.h"

#include <cassert>

#include "Exceptions.h"
#include "ErrorReporter.h"
#include "Util.h"



TypeChecker::TypeChecker(TypeConfiguration config, ErrorReporter& errorReporter)
    :
    config(std::move(config)),
    errorReporter(&errorReporter)
{
}

bool TypeChecker::check(const std::vector<Stmt>& statements)
{
    try {
        identifierTable = IdentifierCollector{ *errorReporter }.collect(statements);
    }
    catch (const std::invalid_argument& err) {
        error({}, err.what());
    }

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

void TypeChecker::operator()(const TypeDef&)
{
    // Nothing
}

void TypeChecker::operator()(const FieldDefinition& def)
{
    const auto& globalObj = std::get<ObjectType>(this->config.types.at(globalObjectTypeName));
    checkFieldDefinition(globalObj, def, true);  // Allow arbitrary fields on the global object
}

auto TypeChecker::getToken(const FieldName& name) -> const Token&
{
    return std::visit([](const auto& v) -> const Token& { return v.name.token; }, name);
}

auto TypeChecker::getToken(const FieldValue& val) -> const Token&
{
    return std::visit(
        [](const auto& v) -> const Token&
        {
            if constexpr (std::same_as<std::remove_cvref_t<decltype(v)>, LiteralValue>) {
                return std::visit([](auto&& v) -> const Token& { return v.token; }, v);
            }
            else {
                return v.token;
            }
        },
        val
    );
}

void TypeChecker::checkFieldDefinition(
    const ObjectType& parent,
    const FieldDefinition& def,
    const bool allowArbitraryFields)
{
    const bool isMapFieldDef = std::holds_alternative<TypedFieldName>(def.name);

    // Get field name and test if a field with that name exists in parent type
    const auto& fieldName = getFieldName(def.name);
    if (parent.hasField(fieldName))
    {
        if (isMapFieldDef && parent.getFieldType(fieldName) != FieldType::eMap)
        {
            error(getToken(def.name), "Can't assign map value to non-map field \"" + fieldName
                                      + "\" on type \"" + parent.typeName + "\"");
        }
    }
    // Test if field is always valid
    else if (!(isMapFieldDef && allowArbitraryFields))
    {
        const auto& token = getToken(def.name);
        error(token, "Invalid field name \"" + fieldName + "\" for type \"" + parent.typeName + "\"");
    }

    // Get the expected type for the field
    const TypeName& expected = [&]{
        if (parent.hasField(fieldName)) return parent.getRequiredType(fieldName);
        return fieldName;
    }();

    // Test if type exists
    if (!config.types.contains(expected)) {
        error(getToken(*def.value), "Expected undefined type \"" + expected + "\".");
    }

    // Test if value is of the expected type
    if (!std::visit(CheckValueType{ config.types.at(expected), *this }, *def.value)) {
        error(getToken(*def.value), "Encountered value of unexpected type.");
    }
}

void TypeChecker::error(const Token& token, std::string message)
{
    throw TypeError{ token, std::move(message) };
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
    return expectedTypeName == std::visit(VariantVisitor{
        [](const StringLiteral&){ return stringTypeName; },
        [](const NumberLiteral& val){
            return std::visit(VariantVisitor{
                [](const double&){ return floatTypeName; },
                [](const int64_t&){ return intTypeName; },
            }, val.value);
        },
    }, val);
}

bool TypeChecker::CheckValueType::operator()(const Identifier& id) const
{
    if (!self->identifierTable.has(id))
    {
        self->error(id.token, "Use of undeclared identifier \"" + id.name + "\"");
        return false;
    }

    return std::visit(VariantVisitor{
        [this, &id](const ValueReference& ref)
        {
            assert(ref.referencedValue != nullptr);
            // Check the referenced value
            return std::visit(CheckValueType{ *this }, *ref.referencedValue);
        },
        [this, &id](const TypeName& name) {
            self->error(id.token, "Expected value but got typename \"" + name + "\".");
            return false;
        },
        [this, &id](const DataConstructor& ctor)
        {
            if (expectedTypeName != ctor.constructedType) {
                expectedTypeError(id.token, ctor.constructedType);
            }
            return true;
        },
    }, self->identifierTable.get(id));
}

bool TypeChecker::CheckValueType::operator()(const ListDeclaration& list) const
{
    /**
     * All values in the list must be of the expected type - we only have
     * homogenous lists.
     */

    bool result{ true };
    for (const auto& fieldValue : list.items) {
        result = result && std::visit(*this, fieldValue);
    }

    return result;
}

bool TypeChecker::CheckValueType::operator()(const ObjectDeclaration& obj) const
{
    if (!std::holds_alternative<ObjectType>(*expectedType)) {
        expectedTypeError(obj.token, undefinedObjectType);
    }

    auto& objectType = std::get<ObjectType>(*expectedType);
    for (const auto& field : obj.fields)
    {
        self->checkFieldDefinition(objectType, field);
    }

    return true;
}

bool TypeChecker::CheckValueType::operator()(const MatchExpression& expr) const
{
    // Test if the matched type exists
    auto it = self->config.types.find(expr.matchedType.name);
    if (it == self->config.types.end())
    {
        self->error(expr.matchedType.token,
                    "Matching on undefined type \"" + expr.matchedType.name + "\".");
    }

    // Check if the matched type is an enum type
    const auto& typeType = it->second;
    if (!std::holds_alternative<EnumType>(typeType))
    {
        self->error(expr.matchedType.token,
                    "Matching on non-enum type \"" + expr.matchedType.name + "\".");
    }
    const auto& enumType = std::get<EnumType>(typeType);

    // Check individual cases
    std::unordered_set<std::string> matchedCases;
    for (const auto& _case : expr.cases)
    {
        // Check the case enumerator
        auto& name = _case.caseIdentifier.name;
        if (!enumType.options.contains(name))
        {
            self->error(_case.caseIdentifier.token,
                        "No option named \"" + name + "\" in enum \"" + expr.matchedType.name + "\".");
        }

        // Check for duplicate matches
        auto [_, success] = matchedCases.emplace(_case.caseIdentifier.name);
        if (!success) {
            self->error(_case.caseIdentifier.token, "Duplicate match on \"" + name + "\".");
        }

        // Check the type of the case's value
        if (!std::visit(CheckValueType{ *this }, *_case.value)) {
            return false;
        }
    }

    // Check for exhaustiveness
    if (matchedCases.size() < enumType.options.size())
    {
        std::string str{ "Non-exhaustive match. Unmatched cases: " };
        for (const auto& opt : enumType.options) {
            if (!matchedCases.contains(opt)) {
                str += opt + ", ";
            }
        }
        self->error(expr.token, str.substr(0, str.size() - 2));
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
