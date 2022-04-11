#pragma once

#include <memory>
#include <filesystem>
namespace fs = std::filesystem;

#include "ErrorReporter.h"

class PipelineDefinitionLanguage
{
public:
    static void run(int argc, char** argv);

private:
    static bool compile(const fs::path& filename);

    static inline std::unique_ptr<ErrorReporter> errorReporter;
};
