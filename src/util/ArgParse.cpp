#include "util/ArgParse.h"

#include <algorithm>
#include <stdexcept>



namespace trc::util
{

auto parseArgs(int argc, const char* argv[]) -> Args
{
    Args result;
    for (int i = 0; i < argc; i++)
    {
        result.emplace_back(argv[i]);
    }

    return result;
}

auto hasNamedArg(const Args& args, const std::string& argName) -> bool
{
    return std::find(args.begin(), args.end(), argName) != args.end();
}

auto getNamedArg(const Args& args, const std::string& argName)
    -> std::pair<std::string, std::string>
{
    auto it = std::find(args.begin(), args.end(), argName);
    if (it != args.end() && it != args.end() - 1)
    {
        return { *it, *(it + 1) };
    }

    throw std::out_of_range("Named argument \"" + argName + "\" does not exist or no value given");
}

auto getNamedArgOr(const Args& args, const std::string& argName, const std::string& defaultValue)
    -> std::pair<std::string, std::string>
{
    try {
        return getNamedArg(args, argName);
    }
    catch (const std::out_of_range&) {
        return { argName, defaultValue };
    }
}

} // namespace trc::util
