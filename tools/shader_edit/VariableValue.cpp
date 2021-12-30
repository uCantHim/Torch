#include "VariableValue.h"



shader_edit::VariableValue::VariableValue(const VariableValue& other)
    :
    source(other.source->copy())
{
}

auto shader_edit::VariableValue::operator=(const VariableValue& other) -> VariableValue&
{
    source = other.source->copy();
    return *this;
}

auto shader_edit::VariableValue::toString() const -> std::string
{
    return source->getCode();
}
