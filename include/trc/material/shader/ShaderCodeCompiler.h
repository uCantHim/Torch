#pragma once

#include <string>
#include <unordered_map>

#include "ShaderCodeBuilder.h"

namespace trc::shader
{
    class ShaderResourceInterfaceBuilder;
    class ShaderRuntimeConstant;

    /**
     * Use the same ShaderValueCompiler object to compile multiple values
     * if they are computed in the same scope.
     */
    class ShaderValueCompiler
    {
    public:
        using Value = ShaderCodeBuilder::Value;

        /**
         * Construct a value compiler.
         */
        explicit ShaderValueCompiler(bool inlineAll = false);

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
        auto operator()(const code::Conditional& v) -> std::string;

    private:
        /** @return std::string Identifier name */
        auto visit(Value val) -> std::string;
        auto genIdentifier() -> std::string;

        const bool inlineAll{ false };

        ui32 nextId{ 0 };

        std::unordered_map<Value, std::string> valueIdentifiers;
        std::string identifierDeclCode;
    };

    class ShaderBlockCompiler
    {
    public:
        using Block = code::Block;

        /**
         * Construct a block compiler.
         */
        ShaderBlockCompiler() = default;

        /**
         * Seed an existing value compiler. Useful if one wants to ensure that
         * generated IDs are unique across multiple blocks.
         */
        explicit ShaderBlockCompiler(ShaderValueCompiler& compiler);

        auto compile(Block block) -> std::string;

        auto operator()(const code::Return& v) -> std::string;
        auto operator()(const code::Assignment& v) -> std::string;
        auto operator()(const code::IfStatement& v) -> std::string;
        auto operator()(const code::FunctionCall& v) -> std::string;

    private:
        ShaderValueCompiler defaultValueCompiler;
        ShaderValueCompiler& valueCompiler{ defaultValueCompiler };
    };
} // namespace trc::shader
