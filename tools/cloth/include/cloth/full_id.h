#pragma once

#include <functional>
#include <optional>
#include <string>

namespace cloth
{
    /**
     * @brief A fully-qualified variable ID, including namespaces.
     */
    struct FullId
    {
        FullId() = default;

        FullId(std::string _fullId, std::string _name, std::optional<std::string> _ns)
            :
            id(std::move(_fullId)),
            name(std::move(_name)),
            nsQualifier(std::move(_ns))
        {}

        /**
         * A variable's fully-qualified identifier, including namespaces.
         *
         * Example: For a declaration `$a:b:myVar`, `id` is "a:b:myVar".
         */
        std::string id;

        /**
         * A variable's namespace-relative identifier.
         *
         * Example: For a declaration `$a:b:myVar`, `name` is "myVar".
         */
        std::string name;

        /**
         * A variable's namespace, if any is specified.
         *
         * Example: For a declaration `$a:b:myVar`, `nsQualifier` is "a:b". For
         *          a declaration `$foo`, `nsQualifier` is std::nullopt.
         */
        std::optional<std::string> nsQualifier;

        /**
         * Note: This is an ignorant function. It does not parse Cloth
         * identifiers, it merely splits a string by the namespace separator.
         * Thus, it will never result in an error.
         */
        static auto fromString(std::string_view str, std::string_view sep = ":") -> FullId
        {
            FullId res;
            res.id = str;
            if (auto pos = str.find_last_of(sep); pos != std::string::npos)
            {
                res.name = str.substr(pos + 1);
                res.nsQualifier = str.substr(0, pos);
            }
            return res;
        }

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
