#include "shader_edit/ConfigParserJson.h"

#include <iostream>

#include "shader_edit/Logger.h"

using namespace shader_edit;



constexpr const char* META_OBJECT_NAME{ "meta" };
constexpr const char* META_BASE_DIR{ "base_dir" };
constexpr const char* META_OUT_DIR{ "out_dir" };

constexpr const char* SHADERS_OBJECT_NAME{ "shaders" };
constexpr const char* SHADERS_VARIABLES{ "variables" };
constexpr const char* SHADERS_PATH{ "path" };

auto parseMeta(const nl::json& json) -> CompileConfiguration::Meta
{
    if (!json.contains(META_OBJECT_NAME)
        || !json.at(META_OBJECT_NAME).is_object())
    {
        warn("Json contains no \"" + std::string(META_OBJECT_NAME)
             + "\" object. Default values will be used.");
        return {};
    }

    const auto& meta = json.at(META_OBJECT_NAME);
    CompileConfiguration::Meta result;

    if (meta.contains(META_BASE_DIR))
    {
        fs::path basePath{ meta.at(META_BASE_DIR) };
        if (fs::is_directory(basePath)) {
            result.basePath = std::move(basePath);
        }
    }
    if (meta.contains(META_OUT_DIR)) {
        result.outDir = fs::path{ meta.at(META_OUT_DIR) };
    }

    return result;
}

auto parseShaders(const nl::json& json) -> std::vector<ShaderFileConfiguration>
{
    auto toVariable = [](const nl::json& json) -> ShaderFileConfiguration::ValueOrVector {
        if (json.is_array())
        {
            return json.get<std::vector<std::string>>();
        }
        return json.get<std::string>();
    };

    if (!json.contains(SHADERS_OBJECT_NAME)
        || !json.at(SHADERS_OBJECT_NAME).is_object())
    {
        warn("Json contains no shader definitions. Parser will generate no output.");
        return {};
    }

    const auto& shaders = json.at(SHADERS_OBJECT_NAME);
    std::vector<ShaderFileConfiguration> result;

    for (const auto& [name, shader] : shaders.items())
    {
        if (!shader.contains(SHADERS_PATH)) continue;
        std::string path = shader.at(SHADERS_PATH).get<std::string>();
        info("Detected shader file \"" + path + "\"\n");

        ShaderFileConfiguration conf{
            .outputFileName=std::move(name),
            .inputFilePath=std::move(path)
        };

        if (shader.contains(SHADERS_VARIABLES)
            && shader.at(SHADERS_VARIABLES).is_object())
        {
            for (const auto& [name, var] : shader.at(SHADERS_VARIABLES).items())
            {
                conf.variables.try_emplace(std::move(name), toVariable(var));
            }
        }

        result.emplace_back(std::move(conf));
    }

    return result;
}



auto shader_edit::parseConfig(const nl::json& json) -> CompileConfiguration
{
    return {
        .meta=parseMeta(json),
        .shaderFiles=parseShaders(json),
    };
}

auto shader_edit::parseConfigJson(std::istream& is) -> CompileConfiguration
{
    return parseConfig(nl::json::parse(is));
}
