#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <variant>
#include <filesystem>

#include <nlohmann/json.hpp>

namespace shader_edit
{
    namespace fs = std::filesystem;
    namespace nl = nlohmann;

    /////////////
    //  Input  //
    /////////////

    struct ShaderFileConfiguration
    {
        using ValueOrVector = std::variant<std::string, std::vector<std::string>>;

        // Meta
        fs::path outputFileName;
        fs::path inputFilePath;

        // Configured data
        std::unordered_map<std::string, ValueOrVector> variables{};
    };

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
        std::string uniqueName;

        std::string code;
    };

    struct CompileResult
    {
        std::vector<CompiledShaderFile> shaderFiles;
    };
} // namespace shader_edit
