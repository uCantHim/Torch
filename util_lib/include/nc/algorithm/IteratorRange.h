#pragma once

#include <memory>

namespace nc::algorithm
{

template<typename T>
class IteratorRange
{
public:
    using value_type = typename T::value_type;

    constexpr inline IteratorRange(T begin, T end)
        : _begin(std::move(begin)), _end(std::move(end))
    {}

    constexpr inline auto begin() const -> T {
        return _begin;
    }

    constexpr inline auto end() const -> T {
        return _end;
    }

private:
    T _begin;
    T _end;
};

} // namespace nc::algorithm
