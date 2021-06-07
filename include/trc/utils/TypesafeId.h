#pragma once

#include <cstdint>
#include <limits>
#include <functional>

namespace trc
{
    template<typename ClassType, std::integral IdType = uint32_t>
    class TypesafeID
    {
    public:
        using Type = IdType;

        /**
         * Maximum value of the underlying numeric type. Can be used to
         * signal an empty value.
         */
        static constexpr IdType NONE{ std::numeric_limits<IdType>::max() };

        constexpr TypesafeID() = default;
        explicit constexpr TypesafeID(IdType id) : _id(id) {}

        template<std::convertible_to<IdType> T>
        explicit constexpr TypesafeID(T id)
            : _id(static_cast<IdType>(id)) {}

        /**
         * Can only be implicitly cast to the ID type, but not to other
         * arithmetic types.
         */
        constexpr operator IdType() const noexcept {
            return _id;
        }

        /**
         * Allow explicit casts to all arithmetic types
         */
        template<typename T>
        explicit constexpr operator T() const noexcept requires std::convertible_to<IdType, T> {
            return static_cast<T>(_id);
        }

    private:
        IdType _id{ static_cast<IdType>(0) };
    };
} // namespace trc


namespace std
{
    /**
     * @brief std::hash specialization for TypesafeID<> template
     */
    template<typename ClassType, typename IdType> requires requires { std::hash<IdType>{}; }
    struct hash<trc::TypesafeID<ClassType, IdType>>
    {
        size_t operator()(const trc::TypesafeID<ClassType, IdType>& id) const noexcept {
            return hash<IdType>{}(id);
        }
    };
} // namespace std
