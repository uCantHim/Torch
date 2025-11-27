#pragma once

#include <optional>
#include <string>
#include <variant>
#include <vector>

#include <trc_util/Padding.h>
#include <trc_util/TypeUtils.h>

#include "BasicType.h"
#include "Constant.h"
#include "trc/Types.h"

namespace trc::shader
{
    struct FunctionType
    {
        std::vector<BasicType> argTypes;
        std::optional<BasicType> returnType;
    };

    namespace code::types
    {
        struct StructTypeT;
        using StructType = s_ptr<const StructTypeT>;
        struct ExternalType;

        /**
         * @brief Any type; either a basic type or a structure type
         */
        using TypeT = std::variant<
            BasicType,
            StructType,
            ExternalType
        >;

        /**
         * @brief Get a type's name
         */
        inline auto to_string(const TypeT& type) -> std::string;

        /**
         * @brief Get a type's size in bytes
         */
        inline auto getTypeSize(const TypeT& type) -> ui32;

        struct ExternalType
        {
            std::string name;
            ui32 size;

            bool operator==(const ExternalType&) const = default;
        };

        struct StructTypeT
        {
            std::string name;
            std::vector<std::pair<TypeT, std::string>> fields;

            auto to_string() const -> const std::string& { return name; }
            auto getName() const -> const std::string& { return name; }

            /**
             * @brief Calculate the type's size in bytes
             */
            auto size() const -> ui32
            {
                ui32 size{ 0 };
                const ui32 padding = _memberPadding();
                for (const auto& [type, _] : fields) {
                    size += util::pad(getTypeSize(type), padding);
                    //size += getTypeSize(type);
                }

                return size;
            }

            auto _memberPadding() const -> ui32
            {
                ui32 padding = 4;
                for (const auto& [type, _] : fields)
                {
                    const auto size = getTypeSize(type);
                    if (size > sizeof(vec2)) {
                        padding = 16;
                    }
                    else if (size > sizeof(int)) {
                        padding = 8;
                    }
                }
                return padding;
            }
        };

        /**
         * @brief Get a type's name
         */
        inline auto to_string(const TypeT& type) -> std::string
        {
            return std::visit(
                util::VariantVisitor{
                    [](BasicType type) { return type.to_string(); },
                    [](s_ptr<const StructTypeT> type) { return type->to_string(); },
                    [](const ExternalType& type) { return type.name; },
                },
                type
            );
        }

        /**
         * @brief Get a type's size in bytes
         */
        inline auto getTypeSize(const TypeT& type) -> ui32
        {
            return std::visit(
                util::VariantVisitor{
                    [](BasicType type) { return type.size(); },
                    [](s_ptr<const StructTypeT> type) { return type->size(); },
                    [](const ExternalType& type) { return type.size; },
                },
                type
            );
        }
    } // namespace code::types

    namespace code
    {
        struct Literal;
        struct Identifier;
        struct FunctionCall;
        struct UnaryOperator;
        struct BinaryOperator;
        struct MemberAccess;
        struct ArrayAccess;
        struct Conditional;

        struct ValueT;

        struct Return;
        struct Assignment;
        struct IfStatement;

        using StmtT = std::variant<
            Return,
            Assignment,
            IfStatement,
            FunctionCall // Re-use the Value-type struct as a statement
        >;

        struct FunctionT;

        struct BlockT
        {
            std::vector<StmtT> statements;
        };

        using Function = s_ptr<const FunctionT>;
        using Block = s_ptr<BlockT>;
        using Value = s_ptr<const ValueT>;
        using Type = types::TypeT;


        // --- Value types --- //

        struct Literal
        {
            Constant value;
        };

        struct Identifier
        {
            std::string name;
        };

        struct FunctionCall
        {
            Function function;
            std::vector<Value> args;
        };

        struct UnaryOperator
        {
            std::string opName;
            Value operand;
        };

        struct BinaryOperator
        {
            std::string opName;
            Value lhs;
            Value rhs;
        };

        struct MemberAccess
        {
            Value lhs;
            Identifier rhs;
        };

        struct ArrayAccess
        {
            Value lhs;
            Value index;
        };

        struct Conditional
        {
            Value condition;
            Value ifTrue;
            Value ifFalse;
        };

        struct ValueT
        {
            std::variant<
                Literal,
                Identifier,
                FunctionCall,
                UnaryOperator,
                BinaryOperator,
                MemberAccess,
                ArrayAccess,
                Conditional
            > value;

            std::optional<Type> typeAnnotation;
        };


        // --- Statement types --- //

        struct Return
        {
            std::optional<Value> val;
        };

        struct Assignment
        {
            code::Value lhs;
            code::Value rhs;
        };

        struct IfStatement
        {
            code::Value condition;
            Block block;
        };


        // --- Function type --- //

        struct FunctionT
        {
            auto getName() const -> const std::string& {
                return name;
            }

            auto getType() const -> const FunctionType& {
                return type;
            }

            auto getArgs() const -> const std::vector<Value>& {
                return argumentRefs;
            }

            auto getBlock() const -> Block {
                return body;
            }

            std::string name;
            FunctionType type;

            Block body;
            std::vector<Value> argumentRefs;
        };

        /**
         * Create a struct type.
         */
        inline
        auto makeStructType(const std::string& name,
                            const std::vector<std::pair<Type, std::string>>& fields)
            -> s_ptr<const types::StructTypeT>
        {
            return std::make_shared<types::StructTypeT>(name, fields);
        }

        /**
         * Create a declaration of an externaly defined type, e.g, a type
         * defined in an included file.
         */
        inline
        auto makeExternalType(std::string name, ui32 size) -> types::ExternalType
        {
            return types::ExternalType{ std::move(name), size };
        }
    } // namespace code
} // namespace trc::shader
