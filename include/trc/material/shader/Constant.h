#pragma once

#include <cstddef>
#include <cstring>

#include <array>
#include <concepts>
#include <string>

#include "BasicType.h"

namespace trc::shader
{
    struct Constant
    {
    public:
        using LargestType = glm::dmat4;
        static constexpr size_t kMaxSize{ sizeof(LargestType) };

        template<std::convertible_to<BasicType> T>
            requires (sizeof(T) <= sizeof(Constant::LargestType))
        Constant(const T& val);

        /**
         * Creates a zero-initialized value by default.
         */
        Constant(BasicType type, std::array<std::byte, kMaxSize> data = {});

        auto getType() const -> BasicType;
        auto datatype() const -> std::string;
        auto toString() const -> std::string;

        template<typename T> requires (sizeof(T) <= sizeof(Constant::LargestType))
        auto as() const -> T;

    private:
        BasicType type;
        std::array<std::byte, kMaxSize> value;
    };

    template<std::convertible_to<BasicType> T>
        requires (sizeof(T) <= sizeof(Constant::LargestType))  // Using Constant::kMaxSize here
                                                               // doesn't compile.
    Constant::Constant(const T& val)
        :
        type(val)
    {
        *reinterpret_cast<T*>(value.data()) = val;
    }

    template<typename T>
        requires (sizeof(T) <= sizeof(Constant::LargestType))
    auto Constant::as() const -> T
    {
        return *reinterpret_cast<const T*>(value.data());
    }
} // namespace trc::shader
