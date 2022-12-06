#pragma once

#include <vector>
#include <string>
#include <istream>
#include <sstream>

namespace trc::util
{
    /**
     * @brief Read lines from a stream
     *
     * @return std::vector<std::string> An array of lines
     */
    auto readLines(std::istream& is) -> std::vector<std::string>;

    /**
     * @brief Split a string
     *
     * @param const std::string& str The string to split
     * @param char delimiter The delimiter at which to split the string.
     *
     * @return List of extracted substrings
     */
	auto splitString(const std::string& to_str, const char delimiter)
        -> std::vector<std::string>;

    /**
     * @brief Split a string
     *
     * @param const std::string& str The string to split
     * @param const std::string& delimiter The delimiter at which to split
     *                                     the string.
     *
     * @return List of extracted substrings
     */
    auto splitString(const std::string& str, const std::string& delimiter)
        -> std::vector<std::string>;

    /**
     * @brief Remove empty strings from a list of strings
     */
    void removeEmpty(std::vector<std::string>& vec);
} // namespace trc::util
