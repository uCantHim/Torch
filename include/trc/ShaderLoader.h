#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>
#include <spirv/CompileSpirv.h>

#include "trc/ShaderPath.h"

namespace trc
{
    namespace fs = std::filesystem;
    namespace nl = nlohmann;

    class ShaderLoader
    {
    public:
        /**
         * @param fs::path indexFile The cache file containing file
         *        modification time information.
         * @param std::vector<fs::path> includePaths Additional include
         *        directories.
         * @param fs::path Output directory for compiled SPIRV binaries.
         */
        ShaderLoader(std::vector<fs::path> includePaths,
                     fs::path binaryPath,
                     std::optional<fs::path> shaderDatabase = std::nullopt,
                     shaderc::CompileOptions opts = makeDefaultOptions());

        static auto makeDefaultOptions() -> shaderc::CompileOptions;

        auto load(ShaderPath shaderPath) -> std::string;

    private:
        struct ShaderDB
        {
        public:
            struct ShaderInfo
            {
                const util::Pathlet source;
                const util::Pathlet target;
                const std::unordered_map<std::string, std::string> variables;
            };

            explicit ShaderDB(nl::json json);

            auto get(std::string_view path) const -> std::optional<ShaderInfo>;

        private:
            nl::json db;
        };

        /**
         * Determines if dst in the dependency src -> dst is dirty and
         * needs recompilation or re-generation.
         */
        static bool binaryDirty(const fs::path& srcPath, const fs::path& binPath);

        /**
         * Search all include paths for a file. Try to look for it in the
         * shader database if no file can be found this way.
         */
        auto findShaderSource(const util::Pathlet& pathlet) const -> std::optional<fs::path>;

        auto compile(const fs::path& srcPath, const fs::path& dstPath) -> std::string;

        std::optional<ShaderDB> shaderDatabase;
        std::vector<fs::path> includePaths;
        fs::path outDir;

        shaderc::CompileOptions compileOpts;
    };
} // namespace trc
