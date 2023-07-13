#pragma once

#include <variant>
#include <vector>

#include <trc_util/Util.h>

#include "BasicType.h"
#include "Constant.h"
#include "ShaderCapabilities.h"
#include "trc/Types.h"

namespace trc
{
    class ShaderCodeBuilder;

    struct FunctionType
    {
        std::vector<BasicType> argTypes;
        std::optional<BasicType> returnType;
    };

    namespace code::types
    {
        struct StructType;

        /**
         * @brief Any type; either a basic type or a structure type
         */
        using TypeT = std::variant<
            BasicType,
            const StructType*
        >;

        struct StructType
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
                for (const auto& [type, _] : fields)
                {
                    size += std::visit(util::VariantVisitor{
                        [](const BasicType& type)  { return type.size(); },
                        [](const StructType* type) { return type->size(); }
                    }, type);
                }

                return size;
            }
        };

        /**
         * @brief Get a type's name
         */
        inline auto to_string(const TypeT& type)
        {
            return std::visit(
                util::VariantVisitor{
                    [](BasicType type)         { return type.to_string(); },
                    [](const StructType* type) { return type->to_string(); }
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

        using Function = const FunctionT*;
        using Block = BlockT*;
        using Value = const ValueT*;
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

        struct ValueT
        {
            std::variant<
                Literal,
                Identifier,
                FunctionCall,
                UnaryOperator,
                BinaryOperator,
                MemberAccess,
                ArrayAccess
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
            friend class ShaderCodeBuilder;

            auto getName() const -> const std::string&;
            auto getType() const -> const FunctionType&;

            auto getArgs() const -> const std::vector<Value>&;
            auto getBlock() const -> Block;

        private:
            friend class ::trc::ShaderCodeBuilder;

            FunctionT(const std::string& _name,
                      FunctionType _type,
                      BlockT* body,
                      std::vector<Value> argRefs);

            std::string name;
            FunctionType type;

            BlockT* body;
            std::vector<Value> argumentRefs;
        };
    } // namespace code
} // namespace trc
