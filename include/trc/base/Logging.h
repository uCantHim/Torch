#pragma once

#include <format>
#include <functional>
#include <ostream>
#include <source_location>
#include <string>

namespace trc
{
    namespace log
    {
        enum class LogLevel
        {
            // Detailed information that is useful to developers.
            eDebug,

            // Messages that confirm or illustrate correct program behaviour.
            eInfo,

            // Potentially dangerous behaviour. The program may not work as
            // intended in the future because of the reasons illustrated here.
            eWarning,

            // Incorrect behaviour that will cause unintended effects, but not
            // be fatal to the overall program execution.
            eError,

            // Unrecoverable errors that will likely cause the program to exit
            // or crash.
            eCritical,
        };

        /**
         * @return bool True if the specified log level is enabled by the
         *              applicable compile-time rules.
         */
        constexpr bool isStaticallyEnabled(LogLevel level) {
            return level >= TRC_LOG_LEVEL;
        }

        /**
         * @brief Set the global log level.
         *
         * Only messages at or above the global log level are logged.
         */
        void setLogLevel(LogLevel level);

        /**
         * @brief Determine whether a log level is currently enabled.
         *
         * @return bool True if `level` is currently enabled by both compile-
         *              time and runtime rules, the latter being the global log
         *              level set by `setLogLevel`.
         */
        bool isEnabled(LogLevel level);
    } // namespace log

    /**
     * @brief A logger.
     *
     * @tparam bool kStaticEnable Enables or disables logging at compile time.
     *                            Can allow lots of compiler optimization for
     *                            statically disabled loggers, reducing runtime
     *                            overhead.
     */
    template<log::LogLevel level, bool kStaticEnable = log::isStaticallyEnabled(level)>
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
                if (this != &other) {
                    other.isOwning = false;
                }
            }

            ~LogEntry()
            {
                if (log::isEnabled(level)) {
                    if (isOwning) os << "\n";
                }
            }

            template<typename T>
            auto operator<<(T&& v) -> LogEntry&
                requires requires (std::ostream& os) { os << std::forward<T>(v); }
            {
                if (log::isEnabled(level)) {
                    os << std::forward<T>(v);
                }
                return *this;
            }

            /**
             * This overload is needed because, apparently, we cannot infer the
             * template argument type when passing a stream manipulator to
             * `operator<<(T&&)`.
             */
            auto operator<<(std::ostream& (*streamManip)(std::ostream&)) -> LogEntry&
            {
                if (log::isEnabled(level)) {
                    os << streamManip;
                }
                return *this;
            }

            auto operator<<(std::source_location loc) -> LogEntry&
            {
                if (log::isEnabled(level))
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

        struct NullLogEntry
        {
            NullLogEntry() = default;
            explicit NullLogEntry(std::ostream&) {}

            auto operator<<(auto&&) -> NullLogEntry& {
                return *this;
            }
            auto operator<<(std::ostream& (*)(std::ostream&)) -> NullLogEntry& {
                return *this;
            }
        };

        using LogEntryT = std::conditional_t<kStaticEnable, LogEntry, NullLogEntry>;

        Logger(const Logger&) = delete;
        Logger& operator=(const Logger&) = delete;

        ~Logger() = default;
        Logger(Logger&&) noexcept = default;
        Logger& operator=(Logger&&) noexcept = default;

        explicit Logger(std::ostream& os) : stream(&os) {}
        Logger(std::ostream& os, std::function<std::string()> header)
            : stream(&os), makeHeader(std::move(header))
        {}

        auto startEntry() -> LogEntryT
        {
            LogEntryT entry{ *stream };
            entry << makeHeader() << " ";
            return entry;
        }

        template<typename T>
        auto operator<<(T&& v) -> LogEntryT
        {
            return std::move(startEntry() << std::forward<T>(v));
        }

        /**
         * @brief A std::format interface.
         */
        template<typename... Args>
        void operator()([[maybe_unused]] std::format_string<Args...> fmt,
                        [[maybe_unused]] Args&&... args)
        {
            if constexpr (kStaticEnable) {
                *stream << std::format(fmt, std::forward<Args>(args)...) << "\n";
            }
        }

        /**
         * @brief A std::format interface.
         */
        template<typename... Args>
        void operator()([[maybe_unused]] const std::locale& loc,
                        [[maybe_unused]] std::format_string<Args...> fmt,
                        [[maybe_unused]] Args&&... args)
        {
            if constexpr (kStaticEnable) {
                *stream << std::format(loc, fmt, std::forward<Args>(args)...) << "\n";
            }
        }

        /**
         * @brief Set the underlying stream to which the logger writes messages.
         */
        void setOutputStream(std::ostream& os)
        {
            stream = &os;
        }

        void setHeader(std::function<std::string()> headerFunc) {
            makeHeader = std::move(headerFunc);
        }

        /**
         * @brief Flush the underlying stream.
         */
        void flush() {
            stream->flush();
        }

    private:
        std::ostream* stream;
        std::function<std::string()> makeHeader{ []{ return ""; } };
    };

    namespace log
    {
        extern Logger<LogLevel::eDebug>    debug;
        extern Logger<LogLevel::eInfo>     info;
        extern Logger<LogLevel::eWarning>  warn;
        extern Logger<LogLevel::eError>    error;
        extern Logger<LogLevel::eCritical> critical;

        /**
         * @brief Insert the current source location into a stream.
         *
         * Example:
         *
         *     log::error << here() << ": An error has occurred!";
         */
        constexpr auto here(std::source_location loc = std::source_location::current()) {
            return loc;
        }

        /**
         * @brief Create a default log header function
         *
         * The header prints the current time and a message severity.
         *
         * @param std::string messageSeverity
         *
         * # Example
         *
         *     std::ofstream myLogFile{ "/var/log/my_log.txt" };
         *     Logger<LogLevel::eError> myLog{ myLogFile, log::makeDefaultLogHeader("ERROR") };
         */
        auto makeDefaultLogHeader(std::string messageSeverity)
            -> std::function<std::string()>;
    }
} // namespace trc
