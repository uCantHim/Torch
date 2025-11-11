#include "parse_utils.h"

#include <cassert>
#include <cctype>

#include <sstream>
#include <string_view>
#include <tuple>
#include <utility>

#include "parser.h"



namespace cloth::util
{

auto findClosingBrace(const std::vector<std::string>& lines,
                      BlockLocation::Loc begin)
    -> std::optional<BlockLocation::Loc>
{
    auto it = lines.begin() + begin.line;
    size_t charPos = begin.pos;

    size_t level = 0;
    while (it != lines.end())
    {
        while (charPos != it->size())
        {
            switch (it->at(charPos))
            {
            case '{': ++level; break;
            case '}':
                if (level == 0) {
                    return BlockLocation::Loc{ static_cast<size_t>(it - lines.begin()), charPos };
                }
                --level;
            }
            ++charPos;
        }
        ++it;
        charPos = 0;
    }

    return std::nullopt;
}

auto findMain(const std::vector<std::string>& lines) -> std::optional<BlockLocation>
{
    using namespace std::string_view_literals;
    using LinesIter = decltype(lines.begin());

    /**
     * Advance to the next non-space character.
     */
    auto skipSpaces = [&](LinesIter it, size_t charPos) -> std::pair<LinesIter, size_t>
    {
        while (it != lines.end())
        {
            auto& line = *it;
            while (charPos < line.size() && std::isspace(line[charPos])) {
                ++charPos;
            }

            // Advance to next line
            if (charPos >= line.size())
            {
                ++it;
                charPos = 0;
                continue;
            }

            return { it, charPos };
        }

        return { it, charPos };
    };

    /**
     * Find a word if it is only separated from the current position by space-
     * like characters.
     */
    auto findFollowingWord = [&](LinesIter it, size_t charPos, std::string_view pattern)
        -> std::optional<std::pair<LinesIter, size_t>>
    {
        std::tie(it, charPos) = skipSpaces(it, charPos);
        if (it->find(pattern, charPos) == charPos) {
            return std::make_pair(it, charPos);
        }
        return std::nullopt;
    };

    /**
     * Find a closing brace for the current nesting level.
     */
    auto findClosingBrace = [&](LinesIter it, size_t charPos)
        -> std::optional<std::pair<LinesIter, size_t>>
    {
        return ::cloth::util::findClosingBrace(lines, { size_t(it - lines.begin()), charPos })
            .transform([&](BlockLocation::Loc loc) {
                return std::make_pair(lines.begin() + loc.line, loc.pos);
            });
    };

    for (auto lineIt = lines.begin(); lineIt != lines.end(); ++lineIt)
    {
        size_t pos = 0;
        while ((pos = lineIt->find("void", pos)) != std::string::npos)
        {
            auto next = pos + 4;

            const auto declBeginLine = static_cast<size_t>(lineIt - lines.begin());
            const auto declBeginChar = pos;
            const auto searchBeginLine = lineIt;
            const auto searchBegin = next;

            const auto wordChain = { "main"sv, "("sv, ")"sv, "{"sv };
            for (auto word : wordChain)
            {
                if (auto hit = findFollowingWord(lineIt, next, word))
                {
                    std::tie(lineIt, next) = *hit;
                    next += word.size();
                }
                else {
                    next = searchBegin;
                    break;
                }
            }

            // Word chain was not found
            if (next == searchBegin)
            {
                lineIt = searchBeginLine;
                pos = next;
                continue;
            }

            // Word chain was found entirely!
            assert(lineIt >= lines.begin());
            BlockLocation mainLoc{
                .declBegin{ declBeginLine, declBeginChar, },
                .declEnd{ lines.size(), lines.back().size() },
                .bodyBegin{ static_cast<size_t>(lineIt - lines.begin()), next, },
                .bodyEnd{ lines.size(), lines.back().size(), },
            };

            if (auto hit = findClosingBrace(lineIt, next))
            {
                auto [it, pos] = *hit;
                mainLoc.bodyEnd = { static_cast<size_t>(it - lines.begin()), pos, };
                mainLoc.declEnd = { static_cast<size_t>(it - lines.begin()), pos + 1, };
                return mainLoc;
            }
            return std::nullopt;
        }
    }

    return std::nullopt;
}

auto formatErrors(const std::vector<parser::Error>& errors,
                  const std::vector<std::string> lines,
                  std::optional<std::string> filePath)
    -> std::string
{
    auto indent = [](size_t n, char c = ' ') { return std::string(n, c); };

    std::stringstream ss;
    for (const auto& err : errors)
    {
        const auto loc = err.location;

        // Error message
        if (filePath) {
            ss << *filePath << ":";
        }
        ss << loc.line + 1 << ":" << loc.firstChar << ": Error: " << err.message << "\n";
        // Code line
        ss << "  " << loc.line + 1 << " | " << lines.at(loc.line) << "\n";
        // Positional indicator line
        ss << "  " << indent(std::to_string(loc.line).size())
                  << " | " << indent(loc.firstChar)
                  << "^" << indent(loc.endChar - loc.firstChar - 1, '~') << "\n";
    }

    return ss.str();
}

} // namespace cloth::util
