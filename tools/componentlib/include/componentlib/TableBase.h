#pragma once

#include <cstdint>
#include <concepts>

namespace componentlib
{

/**
 * T should also be constructible-from and convertible-to size_t, but
 * this does not work when these functions are private with Table declared
 * as a friend, which is the case for ComponentID.
 */
template<typename T>
concept TableKey = std::equality_comparable<T>;

template<typename T>
struct TableTraits;

template<typename Component, TableKey Key = uint32_t>
class Table;

} // namespace componentlib
