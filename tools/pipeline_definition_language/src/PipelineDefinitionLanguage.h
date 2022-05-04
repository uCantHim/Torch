#pragma once

#include <memory>
#include <atomic>
#include <optional>
#include <filesystem>
namespace fs = std::filesystem;

#include "ErrorReporter.h"

struct CompileResult;

class PipelineDefinitionLanguage
{
public:
    static void run(int argc, char** argv);

private:
    static auto compile(const fs::path& filename) -> std::optional<CompileResult>;
    static void writeOutput(const CompileResult& result);

    static void writeShader(const std::string& code, const fs::path& outPath);

    static inline std::atomic<size_t> pendingShaderThreads{ 0 };

    static inline fs::path inputDir{ "." };
    static inline fs::path outputDir{ "." };
    static inline fs::path outputFileName;
    static inline bool outputAsSpirv{ false };
    static inline bool generateHeader{ true };  // Not implemented; always true for now

    static inline std::unique_ptr<ErrorReporter> errorReporter;
};
