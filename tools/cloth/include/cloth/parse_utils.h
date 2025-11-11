#pragma once

#include <optional>
#include <string>
#include <vector>

namespace cloth::parser {
    struct Error;
}

namespace cloth::util
{
    struct BlockLocation
    {
        struct Loc
        {
            size_t line;
            size_t pos;   // The first character
        };

        /// Start of the full block declaration, including name, content, and
        /// opening/closing braces.
        Loc declBegin;

        /// End of the full block declaration, including name, content, and
        /// opening/closing braces.
        Loc declEnd;

        /// Start of the block's content.
        Loc bodyBegin;

        /// End of the block's content.
        Loc bodyEnd;
    };

    auto findMain(const std::vector<std::string>& lines) -> std::optional<BlockLocation>;

    /**
     * Find a closing brace for the current nesting level.
     */
    auto findClosingBrace(const std::vector<std::string>& lines,
                          BlockLocation::Loc begin = { 0, 0 })
        -> std::optional<BlockLocation::Loc>;

    /**
     * Format parser errors with line numbers and code previews.
     */
    auto formatErrors(const std::vector<parser::Error>& errors,
                      const std::vector<std::string> documentLines,
                      std::optional<std::string> filePath)
        -> std::string;
} // namespace cloth::util
