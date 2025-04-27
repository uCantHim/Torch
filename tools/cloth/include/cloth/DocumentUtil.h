#pragma once

#include <optional>
#include <string>
#include <vector>

namespace cloth::util
{
    class DocumentWalker
    {
    public:
        auto getCurrentLocation() const -> std::pair<size_t, size_t>;

        auto peek() -> char;
        auto step() -> char;

        void stepUntilMatch(std::string_view pattern);
        void skipSpaces();
    };

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
