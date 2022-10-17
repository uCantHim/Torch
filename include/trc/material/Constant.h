#pragma once

#include <cstddef>

#include <array>
#include <ostream>
#include <string>

#include "BasicType.h"
#include "trc/Types.h"

namespace trc
{
    struct Constant
    {
        using LargestType = glm::dvec4;

        Constant(i32 val);
        Constant(ui32 val);
        Constant(float val);
        Constant(double val);

        template<int N, typename T> requires (N >= 1 && N <= 4)
        Constant(glm::vec<N, T> value);

        auto datatype() const -> std::string;

        template<typename T> requires (sizeof(T) <= sizeof(LargestType))
        auto as() const -> T;

        BasicType type;
        std::array<std::byte, sizeof(LargestType)> value;
    };

    auto operator<<(std::ostream& os, const Constant& c) -> std::ostream&;

    template<int N, typename T>
        requires (N >= 1 && N <= 4)
    Constant::Constant(glm::vec<N, T> val)
        :
        type(toBasicTypeEnum<T>, N)
    {
        *reinterpret_cast<decltype(val)*>(value.data()) = val;
    }

    template<typename T>
        requires (sizeof(T) <= sizeof(Constant::LargestType))
    auto Constant::as() const -> T
    {
        return *reinterpret_cast<const T*>(value.data());
    }
} // namespace trc
