#ifndef PIPELINECOMPILERUTILS_CPP
#define PIPELINECOMPILERUTILS_CPP

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <sstream>
#include <string>
namespace fs = std::filesystem;

inline auto loadShader(fs::path path) -> std::string
{
    std::stringstream ss;
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("[In loadShader]: Unable to open file " + path.string() + ".");
    }
    ss << file.rdbuf();
    return ss.str();
}

#endif /* end of include guard: PIPELINECOMPILERUTILS_CPP */
