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
    static auto getComponentId() -> uint64_t {
        return id;
    }

private:
    static inline const uint64_t id{ ComponentCrtpBaseIdProvider::nextId++ };
};

/**
 * @brief Ensures that a type is a valid component type
 *
 * It turns out that we don't really need any restrictions, so this is
 * currently a placeholder.
 */
template<typename T>
concept ComponentType = true;

} // namespace componentlib
