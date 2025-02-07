#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "trc/text/GlyphLoading.h"

namespace trc
{
    template<std::invocable<CharCode> F>
    inline constexpr
    void iterUtf8(std::string_view str, F&& func)
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
                code = ((unsigned char)(str[i])     & 0b00000111) << 18
                     | ((unsigned char)(str[i + 1]) & 0b00111111) << 12
                     | ((unsigned char)(str[i + 2]) & 0b00111111) << 6
                     | ((unsigned char)(str[i + 3]) & 0b00111111);
                i += 3;
            }

            func(code);
        }
    }

    /**
     * Transform Unicode code-points into binary-encoded UTF-8
     */
    inline auto encodeUtf8(const std::vector<CharCode>& chars) -> std::string
    {
        std::string result;

        constexpr ui32 fourBytes { 0b111110000000000000000 };
        constexpr ui32 threeBytes{ 0b111111111100000000000 };
        constexpr ui32 twoBytes  { 0b111111111111110000000 };

        constexpr ui32 fourthSixBits{ 0b111111000000000000000000 };
        constexpr ui32 thirdSixBits { 0b111111000000000000 };
        constexpr ui32 secondSixBits{ 0b111111000000 };
        constexpr ui32 firstSixBits { 0b111111 };

        for (CharCode code : chars)
        {
            if (code & fourBytes)
            {
                result += (char)(0b11110000 | (code & fourthSixBits));
                result += (char)(0b10000000 | (code & thirdSixBits));
                result += (char)(0b10000000 | (code & secondSixBits));
                result += (char)(0b10000000 | (code & firstSixBits));
            }
            else if (code & threeBytes)
            {
                result += (char)(0b11100000 | (code & thirdSixBits));
                result += (char)(0b10000000 | (code & secondSixBits));
                result += (char)(0b10000000 | (code & firstSixBits));
            }
            else if (code & twoBytes)
            {
                result += (char)(0b11000000 | (code & secondSixBits));
                result += (char)(0b10000000 | (code & firstSixBits));
            }
            else // Single byte
            {
                result += (char)code;
            }
        }

        return result;
    }
} // namespace trc
