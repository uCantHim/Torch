#include "trc/material/MaterialShaderImpl.h"



namespace trc
{

void MaterialShaderImpl::setParameter(const OutputParameter& output, shader::code::Value value)
{
    parameterValues[output] = value;
}

auto MaterialShaderImpl::getParameterValue(const OutputParameter& param) -> shader::code::Value
{
    return parameterValues.at(param);
}

auto MaterialShaderImpl::tryGetParameterValue(const OutputParameter& param)
    -> std::optional<shader::code::Value>
{
    auto it = parameterValues.find(param);
    if (it != parameterValues.end()) {
        return it->second;
    }
    return std::nullopt;
}

} // namespace trc
