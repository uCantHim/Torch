#pragma once

#include "trc/Types.h"

namespace trc
{
    struct BasicType
    {
        enum class Type : ui8
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

        template<i32 N, typename T>
            requires ((N == 3 || N == 4) && (std::is_same_v<T, float> || std::is_same_v<T, double>))
        BasicType(glm::mat<N, N, T>);

        BasicType(Type t, ui8 channels);

        auto operator<=>(const BasicType&) const = default;

        auto to_string() const -> std::string;

        /** @return ui32 Size of the type in bytes */
        auto size() const -> ui32;

        /** @return ui32 The number of shader locations the type occupies */
        auto locations() const -> ui32;

        Type type;
        ui8 channels;
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

    template<i32 N, typename T>
        requires ((N == 3 || N == 4) && (std::is_same_v<T, float> || std::is_same_v<T, double>))
    BasicType::BasicType(glm::mat<N, N, T>)
        : type(toBasicTypeEnum<T>), channels(N * N)
    {}
} // namespace trc
