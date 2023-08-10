#include "ShaderDatabase.h"

#include <fstream>
#include <mutex>

#include <trc_util/InterProcessLock.h>
#include <trc_util/TypeUtils.h>



auto makeShaderDatabase(ShaderOutputType defaultShaderOutputType, const CompileResult& result)
    -> nl::json
{
    ShaderDatabaseGenerator configGen(defaultShaderOutputType);
    for (const auto& [name, shaders] : result.shaders)
    {
        std::visit(trc::util::VariantVisitor{
            [&, name=name](const ShaderDesc& shader) { configGen.add(UniqueName(name), shader); },
            [&](const VariantGroup<ShaderDesc>& shaders) {
                for (const auto& [name, shader] : shaders.variants) {
                    configGen.add(name, shader);
                }
            },
        }, shaders);
    }
    return configGen.makeConfig();
}

void writeShaderDatabase(const fs::path& path, const nl::json& config, bool append)
{
    trc::util::InterProcessLock sem("/torch_pipeline_compiler_shader_db_lock");
    std::scoped_lock lock(sem);

    if (!fs::is_regular_file(path) || !append)
    {
        std::ofstream file(path);
        file << config;
    }
    else {
        std::ifstream inFile(path);
        assert(inFile.is_open());

        auto db = nl::json::parse(inFile);
        db.merge_patch(config);
        std::ofstream(path) << db;
    }
}



namespace nlohmann
{
    void to_json(nl::json& json, const ShaderDesc& shader)
    {
        json["source"] = shader.source;
        json["target"] = shader.target;
        json["variables"] = shader.variables;
        if (shader.outputType.has_value()) {
            json["outputType"] = shader.outputType.value();
        }
    }
}

ShaderDatabaseGenerator::ShaderDatabaseGenerator(ShaderOutputType defaultOutputType)
    :
    defaultOutputType(defaultOutputType)
{
}

void ShaderDatabaseGenerator::add(const UniqueName& name, ShaderDesc shader)
{
    shader.target = addUniqueExtension(shader.target, name).string();
    if (!shader.outputType) {
        shader.outputType = defaultOutputType;
    }

    shaders.try_emplace(shader.target, std::move(shader));
}

auto ShaderDatabaseGenerator::makeConfig() -> nl::json
{
    return std::move(shaders);
}
