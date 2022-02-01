#pragma once

#include <cstdint>
#include <limits>
#include <functional>

namespace trc::data
{

/**
 * A more typesafe ID type
 */
template<typename ClassType, std::integral IdType = uint32_t>
class TypesafeID
{
public:
    using Type = IdType;

    struct NoneType {};
    static constexpr NoneType NONE{};

    constexpr TypesafeID() = default;
    explicit constexpr TypesafeID(IdType id) : _id(id) {}

    template<std::convertible_to<IdType> T>
    explicit constexpr TypesafeID(T id)
        : _id(static_cast<IdType>(id)) {}

    constexpr TypesafeID(const NoneType&) {
        _setNone();
    }

    /**
     * Can be implicitly cast to the numeric type, but not implicitly
     * constructed from it.
     */
    constexpr operator IdType() const noexcept {
        return _id;
    }

    /**
     * Allow explicit casts to all arithmetic types
     */
    template<std::convertible_to<IdType> T>
    explicit constexpr operator T() const noexcept {
        return static_cast<T>(_id);
    }

    constexpr auto operator<=>(const TypesafeID<ClassType, IdType>&) const = default;

    constexpr bool operator==(const NoneType&) const {
        return _isNone();
    }

    constexpr bool operator!=(const NoneType&) const {
        return !_isNone();
    }

    inline auto operator=(const NoneType&)
    {
        _setNone();
        return *this;
    }

private:
    constexpr bool _isNone() const {
        return _id == std::numeric_limits<IdType>::max();
    }
    constexpr void _setNone() {
        _id = std::numeric_limits<IdType>::max();
    }

    IdType _id{ std::numeric_limits<IdType>::max() };
};

} // namespace trc::data


namespace std
{
    /**
     * @brief std::hash specialization for TypesafeID<> template
     */
    template<typename ClassType, typename IdType> requires requires { std::hash<IdType>{}; }
    struct hash<trc::data::TypesafeID<ClassType, IdType>>
    {
        size_t operator()(const trc::data::TypesafeID<ClassType, IdType>& id) const noexcept {
            return hash<IdType>{}(id);
        }
    };
} // namespace std
