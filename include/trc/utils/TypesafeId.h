#pragma once

#include <cstdint>

namespace trc
{
    template<typename ClassType, typename IdType = uint32_t>
    class TypesafeID
    {
        template<typename T>
        static constexpr bool IsIdType = std::is_convertible_v<IdType, T>;

    public:
        using Type = IdType;

        TypesafeID() = default;
        TypesafeID(IdType id) : _id(id) {}

        template<typename T>
        explicit TypesafeID(T id) requires IsIdType<T>
            : _id(static_cast<IdType>(id)) {}

        /**
         * Can only be implicitly casted to the ID type, but not to other
         * arithmetic types.
         */
        operator IdType() const noexcept {
            return _id;
        }

        /**
         * Allow explicit casts to all arithmetic types
         */
        template<typename T>
        explicit operator T() const noexcept requires IsIdType<T> {
            return static_cast<T>(_id);
        }

    private:
        IdType _id{ static_cast<IdType>(0) };
    };
} // namespace trc
