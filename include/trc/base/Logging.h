#pragma once

#include <ostream>

namespace trc
{
    template<bool Enable>
    class Logger
    {
    public:
        Logger(const Logger&) = delete;
        Logger(Logger&&) = delete;
        Logger& operator=(const Logger&) = delete;
        Logger& operator=(Logger&&) = delete;

        explicit Logger(std::ostream& os) : stream(&os) {}
        ~Logger() = default;

        template<typename T>
        auto operator<<(T&& v) -> Logger&
        {
            if constexpr (Enable) {
                (*stream) << std::forward<T>(v);
            }
            return *this;
        }

        void setOutputStream(std::ostream& os) {
            stream = &os;
        }

    private:
        std::ostream* stream;
    };

    namespace log
    {
#ifdef TRC_DEBUG
        constexpr auto enableDebugLogging = true;
#else
        constexpr auto enableDebugLogging = false;
#endif

        extern Logger<enableDebugLogging> info;
        extern Logger<enableDebugLogging> warn;
        extern Logger<true>               error;
    }
} // namespace trc
