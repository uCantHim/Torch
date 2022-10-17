#include "trc/material/Constant.h"



namespace trc
{

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

auto Constant::datatype() const -> std::string
{
    return type.to_string();
}

auto operator<<(std::ostream& os, const Constant& c) -> std::ostream&
{
    os << c.datatype() << "(" << std::boolalpha;
    for (ui32 i = 0; i < c.type.channels; ++i)
    {
        switch (c.type.type)
        {
        case BasicType::Type::eBool:   os << c.as<glm::bvec4>()[i]; break;
        case BasicType::Type::eSint:   os << c.as<vec4>()[i]; break;
        case BasicType::Type::eUint:   os << c.as<uvec4>()[i]; break;
        case BasicType::Type::eFloat:  os << c.as<vec4>()[i]; break;
        case BasicType::Type::eDouble: os << c.as<glm::dvec4>()[i]; break;
        }
        if (i < c.type.channels - 1) os << ", ";
    }
    os << ")";

    return os;
}

} // namespace trc
