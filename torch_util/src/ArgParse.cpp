#include "trc_util/ArgParse.h"

#include <stdexcept>



namespace trc::util
{

auto parseArgs(int argc, const char* argv[]) -> Args
{
    Args result;
    for (int i = 0; i < argc; i++)
    {
        auto& str = result.emplace_back(argv[i]);

        // Split first equalsign into arg name and value
        if (str.size() < 2 || str[0] != '-' || str[1] != '-') continue;
        for (auto it = str.begin() + 2; it != str.end(); it++)
        {
            if (*it == '=')
            {
                std::string name{ str.begin(), it };
                std::string value{ it + 1, str.end() };
                std::swap(str, name);
                result.emplace_back(std::move(value));
                break;
            }
        }
    }

    return result;
}

auto hasNamedArg(const Args& args, const std::string& argName) -> bool
{
    return std::find(args.begin(), args.end(), argName) != args.end();
}

auto getNamedArg(const Args& args, const std::string& argName) -> std::string
{
    auto it = std::find(args.begin(), args.end(), argName);
    if (it != args.end() && (it + 1) != args.end())
    {
        return *(it + 1);
    }

    throw std::out_of_range("Named argument \"" + argName + "\" does not exist or no value given");
}

auto getNamedArgOr(const Args& args, const std::string& argName, const std::string& defaultValue)
    -> std::string
{
    try {
        return getNamedArg(args, argName);
    }
    catch (const std::out_of_range&) {
        return defaultValue;
    }
}

} // namespace trc::util
