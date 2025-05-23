#pragma once

#include <functional>
#include <iostream>
#include <ranges>
#include <string>
#include <vector>

#include <lsp/types.h>
#include <trc/base/Logging.h>
#include <trc_util/StringManip.h>

// A debug setting for development
constexpr bool _enableLogging = false;

extern trc::Logger<trc::log::LogLevel::eDebug, _enableLogging> debug;

namespace lsp
{
    inline auto operator<<(std::ostream& os, const Position& pos) -> std::ostream&
    {
        os << pos.line << ":" << pos.character;
        return os;
    }

    inline auto operator<<(std::ostream& os, const Range& r) -> std::ostream&
    {
        os << "[" << r.start << ", " << r.end << "]";
        return os;
    }
}

template<>
struct std::hash<lsp::FileURI>
{
    auto operator()(const lsp::FileURI& uri) const -> size_t {
        return std::hash<std::string>{}(uri.toString());
    }
};

/**
 * Splits a string at line boundaries, *keeping newline characters* at the end
 * of lines.
 */
inline auto toLines(const std::string& str) -> std::vector<std::string>
{
    auto res = trc::util::splitString(str, '\n');
    for (auto& line : res | std::views::take(res.size() - 1)) {
        line.push_back('\n');
    }
    return res;
}
