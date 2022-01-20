#pragma once

#include <cstdint>
#include <atomic>

#include "TableBase.h"

namespace componentlib
{

/**
 * @brief Used internally
 */
struct ComponentCrtpBaseIdProvider
{
private:
    template<typename> friend struct ComponentBase;
    static inline uint64_t nextId{ 0 };
};

/**
 * @brief Every component type must derive from this
 */
template<typename Derived>
struct ComponentBase
{
    template<typename, TableKey>
    friend class ComponentStorage;

private:
    static inline const uint64_t id{ ComponentCrtpBaseIdProvider::nextId++ };
};

/**
 * @brief Ensures that a type is a valid component type
 *
 * The constraint used to be `std::derived_from<T, ComponentBase<T>>`, but
 * that doesn't permit plug-and-play for any random type, which I want to
 * have. Especially for third-party types we'd have to create a new class
 * that inherits from the type, but then constructors and other special
 * functions don't work anymore, etc..
 */
template<typename T>
concept ComponentType = true;

} // namespace componentlib
