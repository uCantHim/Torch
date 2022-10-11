#pragma once

#include <map>
#include <string>

#include <nlohmann/json.hpp>
namespace nl = nlohmann;

#include "CompileResult.h"
#include "UniqueName.h"

auto makeShaderDatabase(ShaderOutputType defaultShaderOutputType, const CompileResult& result)
    -> nl::json;

void writeShaderDatabase(const fs::path& path, const nl::json& config, bool append);

class ShaderDatabaseGenerator
{
public:
    ShaderDatabaseGenerator(ShaderOutputType defaultOutputType);

    void add(const UniqueName& name, ShaderDesc shader);

    /**
     * Config format:
     *
     *    <dst_path>: {
     *        "source": <src_path>,
     *        "target": <dst_path>,
     *        "variables": {
     *            <var_name>: <var_value>,
     *            ...
     *        },
     *        "outputType": 0 | 1  (see ShaderOutputType enum)
     *    },
     *    ...
     */
    auto makeConfig() -> nl::json;

private:
    ShaderOutputType defaultOutputType;
    std::map<std::string, ShaderDesc> shaders;
};
