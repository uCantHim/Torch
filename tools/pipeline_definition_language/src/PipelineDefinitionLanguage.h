#pragma once

#include <memory>
#include <atomic>
#include <optional>
#include <filesystem>
namespace fs = std::filesystem;

#include "ErrorReporter.h"
#include "ShaderOutput.h"

namespace shaderc{
    class CompileOptions;
}
struct CompileResult;

class PipelineDefinitionLanguage
{
public:
    static void run(int argc, char** argv);

private:
    static auto compile(const fs::path& filename) -> std::optional<CompileResult>;
    static void writeOutput(const CompileResult& result);

    static void writeShader(const std::string& code,
                            const fs::path& outPath,
                            ShaderOutputType type);
    static void writePlain(const std::string& code, const fs::path& outPath);

    static inline std::atomic<size_t> pendingShaderThreads{ 0 };
#ifdef HAS_SPIRV_COMPILER
    static shaderc::CompileOptions spirvOpts;
#endif

    static inline fs::path outputDir{ "." };
    static inline fs::path shaderInputDir{ "." };
    static inline fs::path shaderOutputDir{ "." };
    static inline fs::path outputFileName;
    static inline ShaderOutputType defaultShaderOutputType{ ShaderOutputType::eGlsl };
    static inline bool generateHeader{ true };  // Not implemented; always true for now

    static inline std::unique_ptr<ErrorReporter> errorReporter;
};
