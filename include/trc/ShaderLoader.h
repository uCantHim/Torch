#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "ShaderPath.h"

namespace trc
{
    namespace fs = std::filesystem;

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
        ShaderLoader(std::vector<fs::path> includePaths, fs::path binaryPath);

        auto load(ShaderPath shaderPath) -> std::string;

    private:
        auto tryLoad(const fs::path& includeDir, const ShaderPath& shaderPath)
            -> std::optional<std::string>;
        auto compile(const fs::path& srcPath, const fs::path& dstPath) -> std::string;

        std::vector<fs::path> includePaths;
        fs::path outDir;
    };
} // namespace trc
