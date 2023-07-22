#pragma once

#include <string>
#include <unordered_map>
#include <variant>

#include "ShaderCodeBuilder.h"

namespace trc
{
    class ShaderResourceInterface;
    class ShaderRuntimeConstant;

    class ResourceResolver
    {
    public:
        virtual ~ResourceResolver() noexcept = default;

        virtual auto resolveCapabilityAccess(Capability cap) -> code::Value = 0;
        virtual auto resolveRuntimeConstantAccess(s_ptr<ShaderRuntimeConstant> c)
            -> code::Value = 0;
    };

    /**
     * Use the same ShaderValueCompiler object to compile multiple values
     * if they are computed in the same scope.
     */
    class ShaderValueCompiler
    {
    public:
        using Value = ShaderCodeBuilder::Value;

        explicit ShaderValueCompiler(ResourceResolver& resolver, bool inlineAll = false);

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
        auto operator()(const code::CapabilityAccess& v) -> std::string;
        auto operator()(const code::RuntimeConstant& v) -> std::string;

    private:
        /** @return std::string Identifier name */
        auto visit(Value val) -> std::string;
        auto genIdentifier() -> std::string;

        const bool inlineAll{ false };
        ResourceResolver* resolver;

        ui32 nextId{ 0 };

        std::unordered_map<Value, std::string> valueIdentifiers;
        std::string identifierDeclCode;
    };

    class ShaderBlockCompiler
    {
    public:
        using Block = code::Block;

        explicit ShaderBlockCompiler(ResourceResolver& resolver);

        auto compile(Block block) -> std::string;

        auto operator()(const code::Return& v) -> std::string;
        auto operator()(const code::Assignment& v) -> std::string;
        auto operator()(const code::IfStatement& v) -> std::string;
        auto operator()(const code::FunctionCall& v) -> std::string;

    private:
        ShaderValueCompiler valueCompiler;
    };
} // namespace trc
