#include "trc/base/Logging.h"

#include <cstdlib>

#include <chrono>
#include <iostream>
#include <format>



namespace trc::log
{
    LogLevel globalLogLevel = []{
        using namespace std::string_view_literals;

        // Override the compile-time choice of the default log level if one is
        // specified by the environment.
        auto envStr = std::getenv(TRC_LOG_LEVEL_ENV_NAME);
        if (!envStr) {
            return TRC_LOG_LEVEL;
        }
        if (envStr == "Critical"sv) {
            return LogLevel::eCritical;
        }
        if (envStr == "Error"sv) {
            return LogLevel::eError;
        }
        if (envStr == "Warning"sv) {
            return LogLevel::eWarning;
        }
        if (envStr == "Info"sv) {
            return LogLevel::eInfo;
        }
        if (envStr == "Debug"sv) {
            return LogLevel::eDebug;
        }
        return TRC_LOG_LEVEL;
    }();

    void setLogLevel(LogLevel level)
    {
        globalLogLevel = level;
    }

    bool isEnabled(LogLevel level)
    {
        return isStaticallyEnabled(level) && level >= globalLogLevel;
    }

    struct DefaultLogHeader
    {
        auto operator()() const -> std::string
        {
            auto time = std::chrono::current_zone()->to_local(std::chrono::system_clock::now());

            // https://en.cppreference.com/w/cpp/chrono/hh_mm_ss/formatter
            // %F is eqivalent to %Y-%m-%d
            // %X is the locale's time representation. %T could work instead but it
            // prints too many sub-second digits for my taste.
            return std::format("[{:%F %X}] --{}-- ", time, severity);
        }

        std::string severity;
    };

    auto makeDefaultLogHeader(std::string messageSeverity) -> std::function<std::string()>
    {
        return DefaultLogHeader{ std::move(messageSeverity) };
    }

    Logger<LogLevel::eDebug>    debug(std::cout, DefaultLogHeader{ "DEBUG" });
    Logger<LogLevel::eInfo>     info(std::cout, DefaultLogHeader{ "INFO" });
    Logger<LogLevel::eWarning>  warn(std::cout, DefaultLogHeader{ "WARNING" });
    Logger<LogLevel::eError>    error(std::cout, DefaultLogHeader{ "ERROR" });
    Logger<LogLevel::eCritical> critical(std::cout, DefaultLogHeader{ "CRITICAL" });
}
