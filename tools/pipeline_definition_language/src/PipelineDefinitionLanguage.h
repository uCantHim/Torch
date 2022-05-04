#pragma once

#include <memory>
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

    static inline fs::path inputDir{ "." };
    static inline fs::path outputDir{ "." };
    static inline fs::path outputFileName;
    static inline bool generateHeader{ true };

    static inline std::unique_ptr<ErrorReporter> errorReporter;
};
