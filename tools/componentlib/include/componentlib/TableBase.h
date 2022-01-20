#pragma once

#include <cstdint>
#include <concepts>

namespace componentlib
{

template<typename T>
concept TableKey = requires {
    std::convertible_to<T, std::size_t>;
    std::constructible_from<T, std::size_t>;
    std::equality_comparable<T>;
};

template<typename T>
struct TableTraits;

template<typename Component, TableKey Key = uint32_t>
class Table;

} // namespace componentlib
