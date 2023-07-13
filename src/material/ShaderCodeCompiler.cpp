#include "trc/material/ShaderCodeCompiler.h"

#include "trc/material/ShaderResourceInterface.h"
#include "trc/material/ShaderTypeChecker.h"



namespace trc
{

using namespace code;

ShaderValueCompiler::ShaderValueCompiler(bool inlineAll)
    :
    inlineAll(inlineAll)
{
}

auto ShaderValueCompiler::compile(Value value) -> std::pair<std::string, std::string>
{
    // visit creates the code that computes the returned identfier's value
    auto identifier = visit(value);

    return { identifier, std::move(identifierDeclCode) };
}

auto ShaderValueCompiler::visit(Value val) -> std::string
{
    // This is the only time that std::visit is called directly, otherwise
    // we always call `this->visit`.
    auto code = std::visit(*this, val->value);

    if (inlineAll) {
        return code;
    }

    // Create a separate assignment to an intermediate variable. This
    // avoids duplicate computations if the same value is referenced
    // multiple times.
    //
    // This only works if the type checker is able to determine a type for
    // the expression! (we don't have `auto` in GLSL)
    if (auto type = ShaderTypeChecker{}.inferType(val))
    {
        auto [it, success] = valueIdentifiers.try_emplace(val);
        if (!success) {
            return it->second;
        }

        const auto id = genIdentifier();
        identifierDeclCode += type->to_string() + " " + id + " = " + std::move(code) + ";\n";
        it->second = id;

        return id;
    }

    // Don't create an assignment for the value if the type checker was
    // unable to determine its type.
    return code;
}

auto ShaderValueCompiler::genIdentifier() -> std::string
{
    return "_id_" + std::to_string(nextId++);
}

auto ShaderValueCompiler::operator()(const Literal& v) -> std::string
{
    return v.value.toString();
}

auto ShaderValueCompiler::operator()(const Identifier& v) -> std::string
{
    return v.name;
}

auto ShaderValueCompiler::operator()(const FunctionCall& v) -> std::string
{
    std::string res = v.function->getName() + "(";
    for (ui32 i = 0; Value arg : v.args)
    {
        res += visit(arg);
        if (++i < v.args.size()) {
            res += ", ";
        }
    }
    res += ")";

    return res;
}

auto ShaderValueCompiler::operator()(const UnaryOperator& v) -> std::string
{
    return "(" + v.opName + visit(v.operand) + ")";
}

auto ShaderValueCompiler::operator()(const BinaryOperator& v) -> std::string
{
    return "(" + visit(v.lhs) + v.opName + visit(v.rhs) + ")";
}

auto ShaderValueCompiler::operator()(const MemberAccess& v) -> std::string
{
    return "(" + visit(v.lhs) + "." + (*this)(v.rhs) + ")";
}

auto ShaderValueCompiler::operator()(const ArrayAccess& v) -> std::string
{
    return visit(v.lhs) + "[" + visit(v.index) + "]";
}



auto ShaderBlockCompiler::compile(Block block) -> std::string
{
    std::string res;
    for (const auto& stmt : block->statements)
    {
        res += std::visit(*this, stmt) + ";\n";
    }

    return res;
}

auto ShaderBlockCompiler::operator()(const Return& v) -> std::string
{
    if (v.val) {
        auto [id, code] = valueCompiler.compile(*v.val);
        return code + "return " + id;
    }
    return "return";
}

auto ShaderBlockCompiler::operator()(const Assignment& v) -> std::string
{
    auto [id, lhsCode] = valueCompiler.compile(v.lhs);
    auto [value, rhsCode] = valueCompiler.compile(v.rhs);

    return lhsCode + rhsCode + id + " = " + value;
}

auto ShaderBlockCompiler::operator()(const IfStatement& v) -> std::string
{
    auto [id, preCode] = valueCompiler.compile(v.condition);
    auto blockCode = compile(v.block);

    return preCode
        + "if (" + id + ")\n{\n"
        + blockCode
        + "}";
}

auto ShaderBlockCompiler::operator()(const FunctionCall& v) -> std::string
{
    code::ValueT value{ .value=v, .typeAnnotation=std::nullopt };
    auto [call, preCode] = valueCompiler.compile(&value);
    return preCode + call;
}

} // namespace trc
