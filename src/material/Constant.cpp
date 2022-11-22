#include "trc/material/Constant.h"

#include <sstream>



namespace trc
{

Constant::Constant(bool val)
    :
    type(BasicType::Type::eBool, 1)
{
    *reinterpret_cast<bool*>(value.data()) = val;
}

Constant::Constant(i32 val)
    :
    type(BasicType::Type::eSint, 1)
{
    *reinterpret_cast<i32*>(value.data()) = val;
}

Constant::Constant(ui32 val)
    :
    type(BasicType::Type::eUint, 1)
{
    *reinterpret_cast<ui32*>(value.data()) = val;
}

Constant::Constant(float val)
    :
    type(BasicType::Type::eFloat, 1)
{
    *reinterpret_cast<float*>(value.data()) = val;
}

Constant::Constant(double val)
    :
    type(BasicType::Type::eDouble, 1)
{
    *reinterpret_cast<double*>(value.data()) = val;
}

Constant::Constant(BasicType type, std::array<std::byte, kMaxSize> data)
    :
    type(type),
    value(data)
{
}

auto Constant::getType() const -> BasicType
{
    return type;
}

auto Constant::datatype() const -> std::string
{
    return type.to_string();
}

auto Constant::toString() const -> std::string
{
    std::stringstream ss;
    ss << *this;
    return ss.str();
}

auto operator<<(std::ostream& os, const Constant& c) -> std::ostream&
{
    os << c.datatype() << "(" << std::boolalpha;
    for (ui8 i = 0; i < c.getType().channels; ++i)
    {
        switch (c.getType().type)
        {
        case BasicType::Type::eBool:   os << c.as<glm::bvec4>()[i]; break;
        case BasicType::Type::eSint:   os << c.as<ivec4>()[i]; break;
        case BasicType::Type::eUint:   os << c.as<uvec4>()[i]; break;
        case BasicType::Type::eFloat:  os << c.as<vec4>()[i]; break;
        case BasicType::Type::eDouble: os << c.as<glm::dvec4>()[i]; break;
        }
        if (i < c.getType().channels - 1) os << ", ";
    }
    os << ")";

    return os;
}

} // namespace trc
