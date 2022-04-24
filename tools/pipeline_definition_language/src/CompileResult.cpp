#include "CompileResult.h"

#include <sstream>



UniqueName::UniqueName(std::string str)
    : uniqueName(std::move(str))
{
}

UniqueName::UniqueName(std::string str, std::vector<VariantFlag> _flags)
    :
    name(std::move(str)),
    flags(std::move(_flags))
{
    std::stringstream ss;
    ss << name;
    for (auto [flag, bit] : flags)
    {
        ss << "_" << flag << ":" << bit;
    }

    uniqueName = ss.str();
}

auto UniqueName::hash() const -> size_t
{
    return std::hash<std::string>{}(uniqueName);
}

bool UniqueName::hasFlags() const
{
    return !flags.empty();
}

auto UniqueName::getFlags() const -> const std::vector<VariantFlag>&
{
    return flags;
}

auto UniqueName::getBaseName() const -> const std::string&
{
    return name;
}

auto UniqueName::getUniqueName() const -> const std::string&
{
    return uniqueName;
}
