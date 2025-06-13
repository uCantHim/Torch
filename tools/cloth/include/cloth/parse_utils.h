#pragma once

#include <optional>
#include <string>
#include <vector>

namespace cloth::util
{
    struct BlockLocation
    {
        struct Loc
        {
            size_t line;
            size_t pos;   // The first character
        };

        Loc declBegin;
        Loc declEnd;
        Loc bodyBegin;
        Loc bodyEnd;
    };

    auto findMain(const std::vector<std::string>& lines) -> std::optional<BlockLocation>;
} // namespace cloth::util
