#pragma once

#include <cstdint>
#include <functional>

namespace trc
{
    template<typename ClassType, typename IdType = uint32_t>
    class TypesafeID
    {
        template<typename T>
        static constexpr bool IsIdType = std::is_convertible_v<IdType, T>;

    public:
        using Type = IdType;

        constexpr TypesafeID() = default;
        explicit constexpr TypesafeID(IdType id) : _id(id) {}

        template<typename T>
        explicit constexpr TypesafeID(T id) requires IsIdType<T>
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
        explicit constexpr operator T() const noexcept requires IsIdType<T> {
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
