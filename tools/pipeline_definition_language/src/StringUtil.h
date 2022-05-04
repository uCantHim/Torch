#pragma once

#include <string>

inline auto capitalize(char c) -> char
{
    if (c >= 'a' && c <= 'z') return c - ('a' - 'A');
    return c;
}

inline auto capitalize(std::string& str) -> std::string&
{
    if (!str.empty()) {
        str[0] = capitalize(str[0]);
    }
    return str;
}

inline auto capitalize(std::string str) -> std::string
{
    if (!str.empty()) {
        str[0] = capitalize(str[0]);
    }
    return str;
}
