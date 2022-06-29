#pragma once

#include <memory>
#include <atomic>
#include <vector>
#include <optional>
#include <filesystem>
namespace fs = std::filesystem;

#ifdef HAS_SPIRV_COMPILER
#include <shaderc/shaderc.hpp>
#endif

#include "ErrorReporter.h"
#include "ShaderOutput.h"

struct CompileResult;

class PipelineDefinitionLanguage
{
public:
    static void run(int argc, char** argv);

private:
    static auto compile(const fs::path& filename) -> std::optional<CompileResult>;
    static void writeOutput(const std::string& uniqueName, const CompileResult& result);

    static void writeShader(const std::string& code,
                            const fs::path& outPath,
                            ShaderOutputType type);
    static void writePlain(const std::string& code, const fs::path& outPath);

    static inline std::atomic<size_t> pendingShaderThreads{ 0 };
#ifdef HAS_SPIRV_COMPILER
    struct SpirvCompileInfo
    {
        std::string shaderCode;
        fs::path outPath;
    };

    static void compileSpirvShaders();
    static void compileToSpirv(const SpirvCompileInfo& info);

    static shaderc::CompileOptions spirvOpts;
    static inline std::vector<SpirvCompileInfo> pendingSpirvCompilations;

    static inline shaderc_spirv_version spirvVersion{ shaderc_spirv_version_1_5 };
    static inline shaderc_target_env targetEnv{ shaderc_target_env_vulkan };
    static inline shaderc_env_version targetEnvVersion{ shaderc_env_version_vulkan_1_2 };
#endif

    static inline fs::path outputDir{ "." };
    static inline fs::path shaderInputDir{ "." };
    static inline fs::path shaderOutputDir{ "." };
    static inline fs::path outputFileName;
    static inline bool generateHeader{ true };  // Not implemented; always true for now

    static inline ShaderOutputType defaultShaderOutputType{ ShaderOutputType::eGlsl };
    static inline std::vector<std::string> shaderCompileDefinitions;

    static inline std::unique_ptr<ErrorReporter> errorReporter;
};
