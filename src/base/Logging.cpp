#include "trc/base/Logging.h"

#include <chrono>
#include <iostream>
#include <format>



namespace trc::log
{
    struct DefaultLogHeader
    {
        explicit DefaultLogHeader(std::string_view severity) : severity(severity) {}

        auto operator()() const -> std::string
        {
            auto time = std::chrono::current_zone()->to_local(std::chrono::system_clock::now());

            // https://en.cppreference.com/w/cpp/chrono/hh_mm_ss/formatter
            // %F is eqivalent to %Y-%m-%d
            // %X is the locale's time representation. %T could work instead but it
            // prints too many sub-second digits for my taste.
            return std::format("[{:%F %X}] --{}-- ", time, severity);
        }

        std::string_view severity;
    };

    auto makeDefaultLogHeader(std::string_view messageSeverity) -> std::function<std::string()>
    {
        return DefaultLogHeader{ messageSeverity };
    }

    Logger<LogLevel::eDebug>   debug(std::cout, DefaultLogHeader{ "DEBUG" });
    Logger<LogLevel::eInfo>    info(std::cout, DefaultLogHeader{ "INFO" });
    Logger<LogLevel::eWarning> warn(std::cout, DefaultLogHeader{ "WARNING" });
    Logger<LogLevel::eError>   error(std::cout, DefaultLogHeader{ "ERROR" });
}
