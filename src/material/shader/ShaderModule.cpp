#include "trc/material/shader/ShaderModule.h"



namespace trc::shader
{

ShaderModule::ShaderModule(
    shader_edit::ShaderDocument _shaderCode,
    ShaderResourceInterface _resourceInfo)
    :
    ShaderResourceInterface(std::move(_resourceInfo)),
    shaderCode(std::move(_shaderCode))
{
}

auto ShaderModule::getShaderCode() const -> const shader_edit::ShaderDocument&
{
    return shaderCode;
}

auto ShaderModule::serialize() const -> serial::ShaderModule
{
    serial::ShaderModule res;
    *res.mutable_resources() = ShaderResourceInterface::serialize();
    res.set_code(this->shaderCode.compile(true));
    return res;
}

auto ShaderModule::deserialize(
    const serial::ShaderModule& mod,
    ShaderRuntimeConstantDeserializer* des)
    -> ShaderModule
{
    return ShaderModule{
        shader_edit::ShaderDocument{ mod.code() },
        ShaderResourceInterface::deserialize(mod.resources(), des),
    };
}

} // namespace trc::shader
