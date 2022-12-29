#include "trc/material/ShaderTypeChecker.h"



namespace trc
{

auto ShaderTypeChecker::getType(Value value) -> std::optional<BasicType>
{
    if (value->typeAnnotation) {
        return *value->typeAnnotation;
    }

    return std::visit(*this, value->value);
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
    if (v.opName == "<"
        || v.opName == "<="
        || v.opName == ">"
        || v.opName == ">="
        || v.opName == "=="
        || v.opName == "!=")
    {
        return bool{};
    }

    const auto r = getType(v.rhs);
    const auto l = getType(v.lhs);

    // No type checking can be performed if either of the operands has an
    // undefined type
    if (!l || !r) {
        return std::nullopt;
    }

    // If none of the types is a matrix, return the bigger of the two types
    if (l->channels <= 4 && r->channels <= 4)
    {
        if (l->channels < r->channels) {
            return r;
        }
        return l;
    }

    // Just get the right-hand-side operand's type because that's correct in
    // the case of matrix-vector multiplication:
    //     mat3(1.0f) * vec3(1, 2, 3) -> vec3
    return getType(v.rhs);
}

auto ShaderTypeChecker::operator()(const code::MemberAccess&)
    -> std::optional<BasicType>
{
    return std::nullopt;
}

auto ShaderTypeChecker::operator()(const code::ArrayAccess& v)
    -> std::optional<BasicType>
{
    return getType(v.lhs);
}

} // namespace trc
