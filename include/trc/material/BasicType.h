#pragma once

#include "trc/Types.h"

namespace trc
{
    struct BasicType
    {
        enum class Type
        {
            eBool,
            eSint,
            eUint,
            eFloat,
            eDouble,
        };

        BasicType(bool val);
        BasicType(i32 val);
        BasicType(ui32 val);
        BasicType(float val);
        BasicType(double val);

        template<i32 N, typename T>
        BasicType(glm::vec<N, T>);

        BasicType(Type t, ui32 channels);

        auto to_string() const -> std::string;

        Type type;
        ui32 channels;
    };

    auto operator<<(std::ostream& os, const BasicType& t) -> std::ostream&;

    template<typename T>
    constexpr BasicType::Type toBasicTypeEnum = BasicType::Type::eFloat;

    template<> inline constexpr BasicType::Type toBasicTypeEnum<bool>   = BasicType::Type::eBool;
    template<> inline constexpr BasicType::Type toBasicTypeEnum<i32>    = BasicType::Type::eSint;
    template<> inline constexpr BasicType::Type toBasicTypeEnum<ui32>   = BasicType::Type::eUint;
    template<> inline constexpr BasicType::Type toBasicTypeEnum<float>  = BasicType::Type::eFloat;
    template<> inline constexpr BasicType::Type toBasicTypeEnum<double> = BasicType::Type::eDouble;



    template<i32 N, typename T>
    BasicType::BasicType(glm::vec<N, T>)
        : type(toBasicTypeEnum<T>), channels(N)
    {}
} // namespace trc
