#pragma once

#include <string>
#include <vector>

/** @brief Enums are translated into these flag types */
struct FlagDesc
{
    std::string flagName;
    std::vector<std::string> flagBits;
};

struct CompileResult
{
    std::vector<FlagDesc> flagTypes;
};
