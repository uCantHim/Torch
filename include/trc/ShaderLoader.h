#pragma once

#include <filesystem>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <nlohmann/json.hpp>
#include <spirv/CompileSpirv.h>

#include "trc/ShaderPath.h"
#include "trc/Types.h"

namespace trc
{
    namespace fs = std::filesystem;
    namespace nl = nlohmann;

    class ShaderLoader
    {
    public:
        /**
         * @param std::vector<fs::path> includePaths Shader include directories.
         * @param fs::path binaryPath Output directory for compiled SPIRV
         *                            binaries.
         * @param std::optional<fs::path> shaderDatabase An optional path to
         *        a shader database file, which caches relationships between
         *        shader sources and their generated binaries.
         * @param shaderc::CompileOptions opts Shader compile options.
         */
        ShaderLoader(std::vector<fs::path> includePaths,
                     fs::path binaryPath,
                     std::optional<fs::path> shaderDatabase = std::nullopt,
                     shaderc::CompileOptions opts = makeDefaultOptions());

        static auto makeDefaultOptions() -> shaderc::CompileOptions;

        /**
         * @brief Load a compiled shader SPIR-V binary from a path.
         *
         * Re-compiles the shader if the binary is outdated.
         */
        auto load(const ShaderPath& shaderPath) const -> std::vector<ui32>;

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
         * Determine if a shader binary is dirty (outdated) and needs
         * recompilation or re-generation.
         */
        bool binaryDirty(const fs::path& srcPath, const fs::path& binPath) const;

        /**
         * Search all include paths for a file. Try to look for it in the
         * shader database if no file can be found this way.
         */
        auto findShaderSource(const util::Pathlet& pathlet) const -> std::optional<fs::path>;

        /**
         * Recursively scan a file for #included files and return their paths.
         *
         * Note: findDeps does not perform path resolution with respect to
         * include directories. Feed it the result of `findShaderSource`.
         */
        auto findDeps(const fs::path& path) const -> std::vector<fs::path>;

        auto compile(const fs::path& srcPath, const fs::path& dstPath) const -> std::vector<ui32>;

        std::optional<ShaderDB> shaderDatabase;
        std::vector<fs::path> includePaths;
        fs::path outDir;

        shaderc::CompileOptions compileOpts;

        // Caches include dependencies for shader source files.
        mutable std::unordered_map<fs::path, std::vector<fs::path>> sourceDepsCache;
        mutable std::mutex sourceDepsCacheLock;
    };
} // namespace trc
