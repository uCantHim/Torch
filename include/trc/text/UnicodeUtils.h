#pragma once

#include <concepts>
#include <string>
#include <vector>

#include "GlyphLoading.h"

namespace trc
{
    template<std::invocable<CharCode> F>
    void iterUtf8(const std::string& str, F func)
    {
        for (size_t i = 0; i < str.size(); i++)
        {
            const char c = str[i];
            CharCode code = 0;

            // Test from lowest byte count to highest because lower characters
            // should be more likely to occur in text

            if (!(c & 0b10000000)) // One byte
            {
                code = (unsigned char)c;
            }
            else if (!(c & 0b00100000)) // Two bytes
            {
                code = ((unsigned char)(str[i])     & 0b00011111) << 6
                     | ((unsigned char)(str[i + 1]) & 0b00111111);
                ++i;
            }
            else if (!(c & 0b00010000)) // Three bytes
            {
                code = ((unsigned char)(str[i])     & 0b00001111) << 12
                     | ((unsigned char)(str[i + 1]) & 0b00111111) << 6
                     | ((unsigned char)(str[i + 2]) & 0b00111111);
                i += 2;
            }
            else if (!(c & 0b00001000)) // Four bytes
            {
                code = ((unsigned int)(str[i])      & 0b00000111) << 18
                     | ((unsigned char)(str[i + 1]) & 0b00111111) << 12
                     | ((unsigned char)(str[i + 2]) & 0b00111111) << 6
                     | ((unsigned char)(str[i + 3]) & 0b00111111);
                i += 3;
            }

            func(code);
        }
    }

    inline auto convertUtf8(const std::string& str) -> std::vector<CharCode>
    {
        std::vector<CharCode> result;
        result.reserve(str.size());
        iterUtf8(str, [&result](CharCode code) { result.push_back(code); });

        return result;
    };
} // namespace trc
