#pragma once

#include <vector>

//#include "text/GlyphLoading.h"
#include "text/UnicodeUtils.h"
#include "ui/event/Event.h"

namespace trc::ui::event
{
    /**
     * Dispatched on InputField elements when an input occurs
     */
    struct Input : EventBase
    {
    public:
        Input(const std::vector<CharCode>& chars) : inputChars(chars) {}

        /**
         * @return std::string UTF-8 encoded string
         */
        inline auto getText() const -> std::string {
            return encodeUtf8(inputChars);
        }

        /**
         * @return const std::vector<CharCode>& Unicode character codes
         */
        inline auto getChars() const -> const std::vector<CharCode>& {
            return inputChars;
        }

    private:
        const std::vector<CharCode> inputChars;
    };
} // namespace trc::ui::event
