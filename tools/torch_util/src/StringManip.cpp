#include "trc_util/StringManip.h"



auto trc::util::readLines(std::istream& is) -> std::vector<std::string>
{
    /**
     * The line
     *
     *    std::getline(is, result.emplace_back())
     *
     * as the loop condition would be much more elegant, but then I always
     * get a trailing empty line.
     */

    std::vector<std::string> result;
    std::string str;
    while (std::getline(is, str)) {
        result.emplace_back(std::move(str));
    }

    return result;
}

auto trc::util::splitString(const std::string& to_str, const char delimiter)
    -> std::vector<std::string>
{
    std::vector<std::string> result;
    std::stringstream _str;
    _str << to_str;
    std::string token;
    while (std::getline(_str, token, delimiter)) {
        result.push_back(std::move(token));
    }

    return result;
}

auto trc::util::splitString(const std::string& str, const std::string& delimiter)
    -> std::vector<std::string>
{
    std::vector<std::string> result;

    auto i = str.begin();
    auto tokenStart = str.begin();
    for (; i != str.end(); i++)
    {
        for (auto delim = delimiter.begin(), temp = i;
            delim != delimiter.end() && temp != str.end();
            delim++, temp++)
        {
            if (*temp != *delim)
                break;
            if (delim == delimiter.end() - 1)
            {
                result.emplace_back(std::string(tokenStart, i));
                tokenStart = temp + 1;
                i = temp; // Not (temp + 1) here because of the i++ in for-loop
            }
        }
    }
    // Append the remainder of the input string
    auto remainingString = std::string(tokenStart, str.end());
    if (!remainingString.empty())
        result.push_back(remainingString);

    return result;
}


void trc::util::removeEmpty(std::vector<std::string>& vec)
{
    for (auto it = vec.begin(); it != vec.end(); it++)
    {
        if (it->empty()) {
            it = --vec.erase(it);
        }
    }
}
