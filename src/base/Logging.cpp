#include "trc/base/Logging.h"

#include <ctime>  // std::localtime

#include <chrono>
#include <iomanip>  // std::put_time
#include <iostream>
#include <sstream>



namespace trc::log
{
    struct DefaultLogHeader
    {
        explicit DefaultLogHeader(std::string_view severity) : severity(severity) {}

        auto operator()() const -> std::string
        {
            std::time_t time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
            std::stringstream ss;
            ss << "[" << std::put_time(std::localtime(&time), "%F %T") << "]"
               << " --" << severity << "-- ";
            return ss.str();
        }

        std::string_view severity;
    };

    Logger<enableDebugLogging> debug(std::cout, DefaultLogHeader{ "DEBUG" });
    Logger<enableDebugLogging> info(std::cout, DefaultLogHeader{ "INFO" });
    Logger<enableDebugLogging> warn(std::cout, DefaultLogHeader{ "WARNING" });
    Logger<true> error(std::cout, DefaultLogHeader{ "ERROR" });
}
