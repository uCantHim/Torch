#include "ConfigParserJson.h"

#include <iostream>

#include "Logger.h"

using namespace shader_edit;



constexpr const char* META_OBJECT_NAME{ "meta" };
constexpr const char* META_BASE_DIR{ "base_dir" };
constexpr const char* META_OUT_DIR{ "out_dir" };

constexpr const char* SHADERS_OBJECT_NAME{ "shaders" };
constexpr const char* SHADERS_VARIABLES{ "variables" };
constexpr const char* SHADERS_INPUT_PATH{ "in_path" };
constexpr const char* SHADERS_OUTPUT_PATH{ "out_path" };

auto determineInputName(const std::string& objectName, const nl::json& shaderObject)
    -> std::string
{
    if (shaderObject.contains(SHADERS_INPUT_PATH)) {
        return shaderObject.at(SHADERS_INPUT_PATH).get<std::string>();
    }
    return objectName;
}

auto determineOutputName(const std::string& objectName, const nl::json& shaderObject)
    -> std::string
{
    if (shaderObject.contains(SHADERS_OUTPUT_PATH)) {
        return shaderObject.at(SHADERS_OUTPUT_PATH).get<std::string>();
    }
    return objectName;
}

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
        ShaderFileConfiguration conf{
            .inputFilePath=determineInputName(name, shader),
            .outputFileName=determineOutputName(name, shader)
        };

        auto varIt = shader.find(SHADERS_VARIABLES);
        if (varIt != shader.end())
        {
            if (!varIt.value().is_object())
            {
                throw ParseError("Variable declaration in " + conf.outputFileName.string()
                                 + " must be an object.");
            }

            for (const auto& [name, permutations] : varIt->items())
            {
                auto [it, success] = conf.variables.try_emplace(name);
                assert(success);  // Should be the case because the json key is unique
                std::vector<ShaderFileConfiguration::Variable>& vec = it->second;
                for (const auto& [tag, value] : permutations.items())
                {
                    if (!value.is_string())
                    {
                        warn("Value " + (tag.empty() ? "" : "\"" + tag + "\" ") + "of variable \""
                             + name + "\" is not a string and will therefore be ignored.");
                        continue;
                    }

                    auto& var = vec.emplace_back();
                    var.tag = tag;
                    var.value = value.get<std::string>();
                }
            }
        }

        result.emplace_back(std::move(conf));
    } // per-shader

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
