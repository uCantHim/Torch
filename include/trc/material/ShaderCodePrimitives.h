#pragma once

#include <variant>
#include <vector>

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

    namespace code
    {
        struct Literal;
        struct Identifier;
        struct FunctionCall;
        struct UnaryOperator;
        struct BinaryOperator;
        struct MemberAccess;
        struct ArrayAccess;

        using ValueT = std::variant<
            Literal,
            Identifier,
            FunctionCall,
            UnaryOperator,
            BinaryOperator,
            MemberAccess,
            ArrayAccess
        >;

        struct Return;

        using StmtT = std::variant<
            Return,
            FunctionCall // Re-use the Value-type struct as a statement
        >;

        struct FunctionT;

        struct BlockT
        {
            std::vector<StmtT> statements;
        };

        using Function = const FunctionT*;
        using Block = const BlockT*;
        using Value = const ValueT*;


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


        // --- Statement types --- //

        struct Return
        {
            std::optional<Value> val;
        };


        // --- Function type --- //

        struct FunctionT
        {
            friend class ShaderCodeBuilder;

            auto getName() const -> const std::string&;
            auto getType() const -> const FunctionType&;

            auto getArgs() const -> const std::vector<Value>&;

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
