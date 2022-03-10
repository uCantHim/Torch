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


/**
 * @brief An extremely typesafe but also very restrictive ID
 *
 * @tparam Friend The only type that can construct a HardTypesafeID from
 *                a numeric value.
 */
template<typename TypeTag, typename Friend, std::integral IdType = uint32_t>
class HardTypesafeID
{
public:
    friend Friend;

    using Type = IdType;

    /**
     * Type for safe comparison with a 'none'-value
     */
    struct NoneType {};
    static constexpr NoneType NONE{};

public:
    /**
     * @brief Construct ID as NONE
     */
    constexpr HardTypesafeID() = default;

    /**
     * @brief Construct ID as NONE
     */
    constexpr HardTypesafeID(const NoneType&) : HardTypesafeID() {}

    /**
     * @brief Set the ID to NONE
     */
    inline auto operator=(const NoneType&)
    {
        _setNone();
        return *this;
    }

    constexpr bool operator==(const HardTypesafeID&) const = default;
    constexpr auto operator<=>(const HardTypesafeID&) const = default;

    constexpr bool operator==(const NoneType&) const {
        return _isNone();
    }

    constexpr bool operator!=(const NoneType&) const {
        return !_isNone();
    }

    constexpr auto hash() const noexcept {
        return std::hash<IdType>{}(_id);
    }

private:
    /**
     * @brief Construct a new ID
     */
    explicit constexpr HardTypesafeID(IdType id) : _id(id) {}

    /**
     * Can be explicitly cast to the numeric type
     */
    explicit constexpr operator IdType() const noexcept {
        return _id;
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

    /**
     * @brief std::hash specialization for HardTypesafeID<> template
     */
    template<typename ClassType, typename Friend, typename IdType>
        requires requires { std::hash<IdType>{}; }
    struct hash<trc::data::HardTypesafeID<ClassType, Friend, IdType>>
    {
        size_t operator()(const trc::data::HardTypesafeID<ClassType, Friend, IdType>& id)
            const noexcept
        {
            return id.hash();
        }
    };
} // namespace std
