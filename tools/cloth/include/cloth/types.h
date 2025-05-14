#pragma once

#include <functional>
#include <string>

namespace cloth
{
    /**
     * @brief A fully-qualified variable ID, including namespaces.
     */
    struct FullId
    {
        std::string id;

        auto operator<=>(const FullId&) const = default;
    };
} // namespace cloth

template<>
struct std::hash<cloth::FullId>
{
    auto operator()(const cloth::FullId& id) const -> size_t {
        return std::hash<std::string>{}(id.id);
    }
};
