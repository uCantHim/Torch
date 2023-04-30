#pragma once

#include <functional>
#include <ostream>
#include <source_location>
#include <string>

namespace trc
{
    template<bool Enable>
    class Logger
    {
    public:
        struct LogEntry
        {
            LogEntry(const LogEntry&) = delete;
            LogEntry& operator=(const LogEntry&) = delete;
            LogEntry& operator=(LogEntry&&) noexcept = delete;

            explicit LogEntry(std::ostream& os) : os(os) {}
            LogEntry(LogEntry&& other) noexcept
                : os(other.os), isOwning(true)
            {
                other.isOwning = false;
            }

            ~LogEntry()
            {
                if constexpr (Enable) {
                    if (isOwning) os << "\n";
                }
            }

            template<typename T>
            auto operator<<(T&& v) -> LogEntry&
            {
                if constexpr (Enable) {
                    os << std::forward<T>(v);
                }
                return *this;
            }

            auto operator<<(std::source_location loc) -> LogEntry&
            {
                if constexpr (Enable)
                {
                    os << "["
                       << loc.file_name() << ":" << loc.line()
                       << ": " << loc.function_name()
                       << "]";
                }
                return *this;
            }

        private:
            std::ostream& os;
            bool isOwning{ true };
        };

        Logger(const Logger&) = delete;
        Logger& operator=(const Logger&) = delete;

        ~Logger() = default;
        Logger(Logger&&) noexcept = default;
        Logger& operator=(Logger&&) noexcept = default;

        explicit Logger(std::ostream& os) : stream(&os) {}
        Logger(std::ostream& os, std::function<std::string()> header)
            : stream(&os), makeHeader(std::move(header))
        {}

        auto startEntry() -> LogEntry
        {
            LogEntry entry{ *stream };
            entry << makeHeader() << " ";
            return entry;
        }

        template<typename T>
        auto operator<<(T&& v) -> LogEntry
        {
            auto entry = startEntry();
            entry << std::forward<T>(v);
            return entry;
        }

        void setOutputStream(std::ostream& os)
        {
            stream = &os;
        }

    private:
        std::ostream* stream;
        std::function<std::string()> makeHeader{ []{ return ""; } };
    };

    namespace log
    {
#ifdef TRC_DEBUG
        constexpr auto enableDebugLogging = true;
#else
        constexpr auto enableDebugLogging = false;
#endif

        extern Logger<enableDebugLogging> debug;
        extern Logger<enableDebugLogging> info;
        extern Logger<enableDebugLogging> warn;
        extern Logger<true>               error;

        inline auto here(std::source_location loc = std::source_location::current())
        {
            return loc;
        }

        /**
         * @brief Create a default log header function
         *
         * The header prints the current time and a message severity.
         *
         * @param std::string_view messageSeverity
         *
         * # Example
         * ```cpp
         *
         * std::ofstream myLogFile{ "/var/log/my_log.txt" };
         * Logger<true> myLog{ myLogFile, log::makeDefaultLogHeader("ERROR") };
         * ```
         */
        auto makeDefaultLogHeader(std::string_view messageSeverity)
            -> std::function<std::string()>;
    }
} // namespace trc
