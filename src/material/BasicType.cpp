#include "trc/material/BasicType.h"



namespace trc
{

auto longTypename(BasicType::Type type) -> std::string
{
    switch (type)
    {
    case BasicType::Type::eBool:   return "bool";
    case BasicType::Type::eSint:   return "int";
    case BasicType::Type::eUint:   return "uint";
    case BasicType::Type::eFloat:  return "float";
    case BasicType::Type::eDouble: return "double";
    }

    throw std::logic_error("");
}

auto shortTypename(BasicType::Type type) -> std::string
{
    switch (type)
    {
    case BasicType::Type::eBool:   return "b";
    case BasicType::Type::eSint:   return "i";
    case BasicType::Type::eUint:   return "u";
    case BasicType::Type::eFloat:  return "";
    case BasicType::Type::eDouble: return "d";
    }

    throw std::logic_error("");
}



BasicType::BasicType(i32)
    : type(Type::eSint), channels(1)
{}

BasicType::BasicType(ui32)
    : type(Type::eUint), channels(1)
{}

BasicType::BasicType(float)
    : type(Type::eFloat), channels(1)
{}

BasicType::BasicType(double)
    : type(Type::eDouble), channels(1)
{}

BasicType::BasicType(Type t, ui32 channels)
    :
    type(t),
    channels(channels)
{
}

auto BasicType::to_string() const -> std::string
{
    assert(channels > 0 && channels <= 4);
    switch (channels)
    {
    case 1: return longTypename(type);
    case 2: return shortTypename(type) + "vec2";
    case 3: return shortTypename(type) + "vec3";
    case 4: return shortTypename(type) + "vec4";
    }

    throw std::logic_error("");
}

auto operator<<(std::ostream& os, const BasicType& t) -> std::ostream&
{
    os << t.to_string();
    return os;
}

} // namespace trc
