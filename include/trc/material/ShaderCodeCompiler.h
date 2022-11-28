#pragma once

#include <string>
#include <unordered_map>
#include <variant>

#include "ShaderCodeBuilder.h"

namespace trc
{
    class ShaderResourceInterface;

    /**
     * Use the same ShaderValueCompiler object to compile multiple values
     * if they are computed in the same scope.
     */
    class ShaderValueCompiler
    {
    public:
        using Value = ShaderCodeBuilder::Value;

        /**
         * @return std::pair<std::string, std::string> [indentifier, declaration code]
         */
        auto compile(Value value) -> std::pair<std::string, std::string>;

        auto operator()(const code::Literal& v) -> std::string;
        auto operator()(const code::Identifier& v) -> std::string;
        auto operator()(const code::FunctionCall& v) -> std::string;
        auto operator()(const code::UnaryOperator& v) -> std::string;
        auto operator()(const code::BinaryOperator& v) -> std::string;
        auto operator()(const code::MemberAccess& v) -> std::string;
        auto operator()(const code::ArrayAccess& v) -> std::string;

    private:
        /** @return std::string Identifier name */
        auto visit(Value val) -> std::string;
        auto genIdentifier() -> std::string;

        ui32 nextId{ 0 };

        std::unordered_map<Value, std::string> valueIdentifiers;
        std::string identifierDeclCode;
    };

    class ShaderBlockCompiler
    {
    public:
        using Block = code::Block;

        auto compile(Block block) -> std::string;

        auto operator()(const code::Return& v) -> std::string;
        auto operator()(const code::Assignment& v) -> std::string;
        auto operator()(const code::IfStatement& v) -> std::string;
        auto operator()(const code::FunctionCall& v) -> std::string;

    private:
        ShaderValueCompiler valueCompiler;
    };
} // namespace trc
