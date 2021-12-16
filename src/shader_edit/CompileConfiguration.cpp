#include "shader_edit/CompileConfiguration.h"

#include "shader_edit/ConfigParserJson.h"



auto shader_edit::CompileConfiguration::fromJson(const nl::json& json)
    -> CompileConfiguration
{
    return parseConfig(json);
}

auto shader_edit::CompileConfiguration::fromJson(std::istream& is)
    -> CompileConfiguration
{
    return parseConfigJson(is);
}
