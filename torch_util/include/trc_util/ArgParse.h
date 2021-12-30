#pragma once

#include <string>
#include <vector>

namespace trc::util
{
    using Args = std::vector<std::string>;

    auto parseArgs(int argc, const char* argv[]) -> Args;

    auto hasNamedArg(const Args& args, const std::string& argName) -> bool;

    auto getNamedArg(const Args& args, const std::string& argName) -> std::string;

    auto getNamedArgOr(const Args& args,
                       const std::string& argName,
                       const std::string& defaultValue)
        -> std::string;
} // namespace trc::util
