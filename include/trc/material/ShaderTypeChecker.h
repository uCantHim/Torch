#pragma once

#include <optional>

#include "ShaderCodeBuilder.h"

namespace trc
{

class ShaderTypeChecker
{
public:
    using Value = ShaderCodeBuilder::Value;

    auto getType(Value value) -> std::optional<BasicType>;

    auto operator()(const code::Literal& v) -> std::optional<BasicType>;
    auto operator()(const code::Identifier& v) -> std::optional<BasicType>;
    auto operator()(const code::FunctionCall& v) -> std::optional<BasicType>;
    auto operator()(const code::UnaryOperator& v) -> std::optional<BasicType>;
    auto operator()(const code::BinaryOperator& v) -> std::optional<BasicType>;
    auto operator()(const code::MemberAccess& v) -> std::optional<BasicType>;
    auto operator()(const code::ArrayAccess& v) -> std::optional<BasicType>;
};

} // namespace trc
