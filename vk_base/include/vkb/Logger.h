#pragma once

#include <string>
#include <fstream>
#include <exception>

namespace vkb
{

constexpr const char* DEFAULT_LOG = { "logs\\default.log" };

class Logger
{
public:
    explicit
    Logger(const std::string& path = DEFAULT_LOG);
    Logger(const Logger&) = delete;
    Logger(Logger&&) = default;
    ~Logger() noexcept;

    Logger& operator=(const Logger&) = delete;
    Logger& operator=(Logger&&) = default;

    void log(const std::string& logMsg);

    static void log(const std::string& logMsg, const std::string& path);

private:
    std::ofstream logFile;
};

} // namespace vkb
