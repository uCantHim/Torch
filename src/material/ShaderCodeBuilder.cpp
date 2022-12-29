#include "trc/material/ShaderCodeBuilder.h"

#include <sstream>

#include "trc/material/ShaderCodeCompiler.h"
#include "trc/material/ShaderTypeChecker.h"



namespace trc
{

using namespace code;

void ShaderCodeBuilder::startBlock(Function func)
{
    blockStack.push(func->getBlock());
}

void ShaderCodeBuilder::startBlock(Block block)
{
    blockStack.push(block);
}

void ShaderCodeBuilder::endBlock()
{
    assert(!blockStack.empty());
    blockStack.pop();
}

void ShaderCodeBuilder::makeReturn()
{
    makeStatement(Return{});
}

void ShaderCodeBuilder::makeReturn(Value retValue)
{
    makeStatement(Return{ retValue });
}

auto ShaderCodeBuilder::makeConstant(Constant c) -> Value
{
    return makeValue(Literal{ .value=c });
}

auto ShaderCodeBuilder::makeCall(Function func, std::vector<Value> args) -> Value
{
    return makeValue(FunctionCall{ .function=func, .args=args });
}

auto ShaderCodeBuilder::makeMemberAccess(Value val, const std::string& member) -> Value
{
    return makeValue(MemberAccess{ .lhs=val, .rhs=Identifier{ member } });
}

auto ShaderCodeBuilder::makeArrayAccess(Value array, Value index) -> Value
{
    return makeValue(ArrayAccess{ .lhs=array, .index=index });
}

auto ShaderCodeBuilder::makeExternalCall(const std::string& funcName, std::vector<Value> args)
    -> Value
{
    return makeCall(makeOrGetBuiltinFunction(funcName), std::move(args));
}

auto ShaderCodeBuilder::makeExternalIdentifier(const std::string& id) -> Value
{
    return makeValue(Identifier{ id });
}

auto ShaderCodeBuilder::makeNot(Value val) -> Value
{
    return makeValue(UnaryOperator{ "!", val });
}

auto ShaderCodeBuilder::makeAdd(Value lhs, Value rhs) -> Value
{
    return makeValue(BinaryOperator{ .opName="+", .lhs=lhs, .rhs=rhs });
}

auto ShaderCodeBuilder::makeSub(Value lhs, Value rhs) -> Value
{
    return makeValue(BinaryOperator{ .opName="-", .lhs=lhs, .rhs=rhs });
}

auto ShaderCodeBuilder::makeMul(Value lhs, Value rhs) -> Value
{
    return makeValue(BinaryOperator{ .opName="*", .lhs=lhs, .rhs=rhs });
}

auto ShaderCodeBuilder::makeDiv(Value lhs, Value rhs) -> Value
{
    return makeValue(BinaryOperator{ .opName="/", .lhs=lhs, .rhs=rhs });
}

auto ShaderCodeBuilder::makeSmallerThan(Value lhs, Value rhs) -> Value
{
    return makeValue(BinaryOperator{ .opName="<", .lhs=lhs, .rhs=rhs });
}

auto ShaderCodeBuilder::makeGreaterThan(Value lhs, Value rhs) -> Value
{
    return makeValue(BinaryOperator{ .opName=">", .lhs=lhs, .rhs=rhs });
}

auto ShaderCodeBuilder::makeSmallerOrEqual(Value lhs, Value rhs) -> Value
{
    return makeValue(BinaryOperator{ .opName="<=", .lhs=lhs, .rhs=rhs });
}

auto ShaderCodeBuilder::makeGreaterOrEqual(Value lhs, Value rhs) -> Value
{
    return makeValue(BinaryOperator{ .opName=">=", .lhs=lhs, .rhs=rhs });
}

auto ShaderCodeBuilder::makeEqual(Value lhs, Value rhs) -> Value
{
    return makeValue(BinaryOperator{ .opName="==", .lhs=lhs, .rhs=rhs });
}

auto ShaderCodeBuilder::makeNotEqual(Value lhs, Value rhs) -> Value
{
    return makeValue(BinaryOperator{ .opName="!=", .lhs=lhs, .rhs=rhs });
}

auto ShaderCodeBuilder::makeFunction(
    const std::string& name,
    FunctionType type) -> Function
{
    auto block = blocks.emplace_back(std::make_unique<BlockT>()).get();

    // Create argument identifiers
    std::vector<Value> argumentRefs;
    for (ui32 i = 0; i < type.argTypes.size(); ++i)
    {
        std::string argName = "_arg_" + std::to_string(i);
        argumentRefs.emplace_back(makeValue(Identifier{ .name=std::move(argName) }));
    }

    return makeFunction(FunctionT{ name, std::move(type), block, std::move(argumentRefs) });
}

auto ShaderCodeBuilder::getFunction(const std::string& funcName) const -> std::optional<Function>
{
    auto it = functions.find(funcName);
    if (it != functions.end()) {
        return it->second.get();
    }
    return std::nullopt;
}

void ShaderCodeBuilder::makeAssignment(code::Value lhs, code::Value rhs)
{
    makeStatement(code::Assignment{ .lhs=lhs, .rhs=rhs });
}

void ShaderCodeBuilder::makeCallStatement(Function func, std::vector<code::Value> args)
{
    makeStatement(code::FunctionCall{ func, std::move(args) });
}

void ShaderCodeBuilder::makeExternalCallStatement(
    const std::string& funcName,
    std::vector<code::Value> args)
{
    Function func = makeOrGetBuiltinFunction(funcName);
    makeStatement(code::FunctionCall{ func, std::move(args) });
}

auto ShaderCodeBuilder::makeIfStatement(Value condition) -> Block
{
    Block block = blocks.emplace_back(std::make_unique<BlockT>()).get();
    makeStatement(IfStatement{ condition, block });

    return block;
}

template<typename T>
auto ShaderCodeBuilder::makeValue(T&& val) -> Value
{
    return values.emplace_back(std::make_unique<ValueT>(
        ValueT{ .value=std::forward<T>(val), .typeAnnotation=std::nullopt }
    )).get();
}

void ShaderCodeBuilder::makeStatement(StmtT statement)
{
    if (blockStack.empty()) {
        throw std::runtime_error("[In ShaderCodeBuilder]: Cannot create a statement when no"
                                 " block is active. Create a block with `startBlock` first.");
    }

    blockStack.top()->statements.emplace_back(std::move(statement));
}

auto ShaderCodeBuilder::makeFunction(FunctionT func) -> Function
{
    auto [it, _] = functions.try_emplace(func.getName(), std::make_unique<FunctionT>(func));
    return it->second.get();
}

auto ShaderCodeBuilder::makeOrGetBuiltinFunction(const std::string& funcName) -> Function
{
    auto [it, _] = builtinFunctions.try_emplace(
        funcName,
        std::make_unique<FunctionT>(FunctionT{ funcName, {}, nullptr, {} })
    );
    return it->second.get();
}

void ShaderCodeBuilder::annotateType(Value val, BasicType type)
{
    ((ValueT*)val)->typeAnnotation = type;
}

auto ShaderCodeBuilder::compileFunctionDecls() -> std::string
{
    std::string res;
    for (const auto& [name, func] : functions)
    {
        auto& type = func->getType();
        res += (type.returnType ? type.returnType->to_string() : "void")
            + " " + func->getName() + "(";

        for (ui32 i = 0; const auto& argType : type.argTypes)
        {
            Value arg = func->getArgs()[i];
            assert(std::holds_alternative<Identifier>(arg->value));
            res += argType.to_string() + " " + std::get<Identifier>(arg->value).name;
            if (++i < type.argTypes.size()) {
                res += ", ";
            }
        }
        res += ")\n{\n" + compile(func->body) + "}\n";
    }

    return res;
}

auto ShaderCodeBuilder::compile(Value value) -> std::pair<std::string, std::string>
{
    ShaderValueCompiler compiler;
    return compiler.compile(value);
}

auto ShaderCodeBuilder::compile(Block block) -> std::string
{
    ShaderBlockCompiler compiler;
    return compiler.compile(block);
}



code::FunctionT::FunctionT(
    const std::string& _name,
    FunctionType _type,
    BlockT* body,
    std::vector<Value> argRefs)
    :
    name(_name),
    type(std::move(_type)),
    body(body),
    argumentRefs(std::move(argRefs))
{
}

auto code::FunctionT::getName() const -> const std::string&
{
    return name;
}

auto code::FunctionT::getType() const -> const FunctionType&
{
    return type;
}

auto code::FunctionT::getArgs() const -> const std::vector<Value>&
{
    return argumentRefs;
}

auto code::FunctionT::getBlock() const -> Block
{
    return body;
}

} // namespace trc
