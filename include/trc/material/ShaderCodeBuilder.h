#pragma once

#include <concepts>
#include <functional>
#include <stack>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include "Constant.h"
#include "ShaderCodePrimitives.h"
#include "trc/Types.h"

namespace trc
{
    class ShaderCodeBuilder
    {
    public:
        using Function = code::Function;
        using Block = code::Block;
        using Value = code::Value;

        void startBlock(Function function);
        void endBlock();

        void makeReturn();
        void makeReturn(Value retValue);

        auto makeConstant(Constant c) -> Value;
        auto makeCall(Function func, std::vector<Value> args) -> Value;
        auto makeMemberAccess(Value val, const std::string& member) -> Value;
        auto makeArrayAccess(Value array, Value index) -> Value;

        auto makeExternalIdentifier(const std::string& id) -> Value;
        auto makeExternalCall(const std::string& funcName, std::vector<Value> args) -> Value;

        /** @brief Create a unary boolean NOT operation */
        auto makeNot(Value val) -> Value;
        auto makeAdd(Value lhs, Value rhs) -> Value;
        auto makeSub(Value lhs, Value rhs) -> Value;
        auto makeMul(Value lhs, Value rhs) -> Value;
        auto makeDiv(Value lhs, Value rhs) -> Value;

        auto makeFunction(const std::string& name, FunctionType type) -> Function;
        auto getFunction(const std::string& name) -> std::optional<Function>;

        auto compileFunctionDecls() -> std::string;

        /**
         * @return pair [<identifier>, <code>] where <identifier> is a GLSL
         *         variable name that refers to the computed value, and
         *         <code> is GLSL code that declares some intermediate
         *         variables requried for the computation of <identfier>
         *         as well as <identfier> itself.
         *         <code> *must* precede any use of <identifier> in subsequent
         *         code.
         */
        static auto compile(Value value) -> std::pair<std::string, std::string>;
        static auto compile(Block block) -> std::string;

    private:
        friend class ShaderValueCompiler;
        friend class ShaderBlockCompiler;
        friend class ShaderTypeChecker;

        template<typename T>
        auto makeValue(T&& val) -> Value;
        void makeStatement(code::StmtT statement);
        auto makeFunction(code::FunctionT func) -> Function;

        std::vector<u_ptr<code::ValueT>> values;
        std::unordered_map<std::string, u_ptr<code::FunctionT>> functions;
        std::unordered_map<std::string, u_ptr<code::FunctionT>> builtinFunctions;

        std::vector<u_ptr<code::BlockT>> blocks;

        /**
         * The block stack does not necessarily signify block nesting in
         * the code - it just remembers which block is currently being
         * operated on when creating statements.
         */
        std::stack<code::BlockT*> blockStack;
    };
} // namespace trc
