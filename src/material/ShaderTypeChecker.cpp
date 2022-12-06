#include "trc/material/ShaderTypeChecker.h"



namespace trc
{

auto ShaderTypeChecker::getType(Value value) -> std::optional<BasicType>
{
    return std::visit(*this, *value);
}

auto ShaderTypeChecker::operator()(const code::Literal& v)
    -> std::optional<BasicType>
{
    return v.value.getType();
}

auto ShaderTypeChecker::operator()(const code::Identifier&)
    -> std::optional<BasicType>
{
    return std::nullopt;
}

auto ShaderTypeChecker::operator()(const code::FunctionCall& v)
    -> std::optional<BasicType>
{
    return v.function->getType().returnType;
}

auto ShaderTypeChecker::operator()(const code::UnaryOperator& v)
    -> std::optional<BasicType>
{
    return getType(v.operand);
}

auto ShaderTypeChecker::operator()(const code::BinaryOperator& v)
    -> std::optional<BasicType>
{
    assert(!(getType(v.lhs) && getType(v.rhs)) || getType(v.lhs)->type == getType(v.rhs)->type);

    if (v.opName == "<"
        || v.opName == "<="
        || v.opName == ">"
        || v.opName == ">="
        || v.opName == "=="
        || v.opName == "!=")
    {
        return bool{};
    }

    return getType(v.lhs);
}

auto ShaderTypeChecker::operator()(const code::MemberAccess&)
    -> std::optional<BasicType>
{
    return std::nullopt;
}

auto ShaderTypeChecker::operator()(const code::ArrayAccess& v)
    -> std::optional<BasicType>
{
    return std::visit(*this, *v.lhs);
}

} // namespace trc
