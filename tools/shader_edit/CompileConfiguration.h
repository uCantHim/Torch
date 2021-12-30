#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <variant>
#include <filesystem>

#include <nlohmann/json.hpp>

#include "VariableValue.h"

namespace shader_edit
{
    namespace fs = std::filesystem;
    namespace nl = nlohmann;

    /////////////
    //  Input  //
    /////////////

    struct ShaderFileConfiguration
    {
        struct Variable
        {
            std::string tag;
            VariableValue value;
        };
        using VariableName = std::string;

        // Meta
        fs::path outputFileName;
        fs::path inputFilePath;

        // Configured data
        std::unordered_map<VariableName, std::vector<Variable>> variables{};
    };

    inline auto render(const ShaderFileConfiguration::Variable& var) -> std::string
    {
        return var.value.toString();
    }

    struct CompileConfiguration
    {
        /**
         * @param const nl::json& json
         *
         * @return CompileConfiguration
         */
        static auto fromJson(const nl::json& json) -> CompileConfiguration;

        /**
         * @param std::istream& is
         *
         * @return CompileConfiguration
         */
        static auto fromJson(std::istream& is) -> CompileConfiguration;

        struct Meta
        {
            // Input directory relative to which shader paths are evaluated
            fs::path basePath{ "." };

            // Output directory to which compilation results are written
            fs::path outDir{ "." };
        };

        Meta meta;
        std::vector<ShaderFileConfiguration> shaderFiles{};
    };



    //////////////
    //  Output  //
    //////////////

    struct CompiledShaderFile
    {
        fs::path filePath;
        std::string code;
    };

    struct CompileResult
    {
        std::vector<CompiledShaderFile> shaderFiles;
    };
} // namespace shader_edit
