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
concept TableKey = std::equality_comparable<T>
                && std::constructible_from<T, std::size_t>
                && requires (T a) { static_cast<std::size_t>(a); };

template<typename T, TableKey Key, typename>
class Table;

} // namespace componentlib
