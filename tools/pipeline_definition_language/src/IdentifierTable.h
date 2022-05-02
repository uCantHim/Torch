#pragma once

#include <vector>
#include <unordered_map>

#include "Exceptions.h"
#include "SyntaxElements.h"

class ErrorReporter;

struct ValueReference
{
    const FieldValue* referencedValue;
};

using TypeName = std::string;

struct DataConstructor
{
    TypeName constructedType;
    std::string dataName;
};

/**
 * @brief The type of entity that is associated with an identifier
 *
 * An identifier can have different semantics depending on context:
 *
 *  - ValueReference: The identifier refers to a value which is defined as
 *                    a variable somewhere else in the code.
 *
 *  - TypeName: The identifier is the name of a type.
 *
 *  - DataConstructor: The identifier is the name of a data constructor
 *                     that constructs a value of a specific type.
 */
using IdentifierValue = std::variant<
    ValueReference,
    TypeName,
    DataConstructor
>;

/** @brief Signals that a duplicate identifier definition has been found */
struct DuplicateIdentifierError : public PipelineLanguageCompilerError
{
    DuplicateIdentifierError(Identifier id) : identifier(std::move(id)) {}

    Identifier identifier;
};

/**
 * @brief Saves top-level variable declarations with their values
 */
class IdentifierTable
{
public:
    /**
     * @throw DuplicateIdentifierError if the identifier `id` already
     *                                 exists in the table.
     */
    void registerIdentifier(const Identifier& id, IdentifierValue val);

    bool has(const Identifier& id) const;

    /**
     * @throw std::out_of_range if `id` does not exist in the table
     */
    auto get(const Identifier& id) const -> const IdentifierValue&;

    auto getValueReference(const Identifier& id) const -> const ValueReference*;
    auto getTypeName(const Identifier& id) const -> const TypeName*;
    auto getDataConstructor(const Identifier& id) const -> const DataConstructor*;

private:
    template<typename T>
    auto get(const Identifier& id) const -> const T*;

    std::unordered_map<Identifier, IdentifierValue> table;
};

/**
 * @brief Collects all top-level values qualified with a typed name
 * ('global variables') and saves them in a lookup table.
 */
class IdentifierCollector
{
public:
    explicit IdentifierCollector(ErrorReporter& errorReporter);

    auto collect(const std::vector<Stmt>& statements) -> IdentifierTable;

    void operator()(const TypeDef& def);
    void operator()(const EnumTypeDef& def);
    void operator()(const FieldDefinition&);

private:
    IdentifierTable table;
    ErrorReporter* errorReporter;
};
