#include "UniqueName.h"

#include <sstream>



auto addUniqueExtension(fs::path file, const UniqueName& uniqueName) -> fs::path
{
    return file.replace_extension(uniqueName.getUniqueExtension() + file.extension().string());
}



UniqueName::UniqueName(std::string str)
    :
    name(str),
    uniqueName(std::move(str))
{
}

UniqueName::UniqueName(std::string str, VariantFlagSet _flags)
    :
    name(std::move(str)),
    flags(std::move(_flags)),
    uniqueName(name + getUniqueExtension())
{
}

auto UniqueName::hash() const -> size_t
{
    return std::hash<std::string>{}(uniqueName);
}

bool UniqueName::hasFlags() const
{
    return !flags.empty();
}

auto UniqueName::getFlags() const -> const VariantFlagSet&
{
    return flags;
}

auto UniqueName::calcFlagIndex(const FlagTable& flagTable) const -> size_t
{
    uint32_t index{ 0 };
    uint32_t totalMax{ 1 };
    for (auto& flag : getFlags())
    {
        index += totalMax * flag.flagBitId;
        totalMax *= flagTable.getNumFlagBits(flag);
    }
    return index;
}

auto UniqueName::getBaseName() const -> const std::string&
{
    return name;
}

auto UniqueName::getUniqueName() const -> const std::string&
{
    return uniqueName;
}

auto UniqueName::getUniqueExtension() const -> std::string
{
    std::stringstream ss;
    for (auto [flag, bit] : flags) {
        ss << "_" << flag << ":" << bit;
    }

    return ss.str();
}
