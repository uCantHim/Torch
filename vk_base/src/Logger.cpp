#include "Logger.h"

#include <iostream>



vkb::Logger::Logger(const std::string& path)
{
    logFile.open(path, std::ios_base::out);
    if (!logFile.is_open())
        throw std::runtime_error("Unable to open file " + path + ".");
}


vkb::Logger::~Logger() noexcept
{
    logFile.close();
}


void vkb::Logger::log(const std::string& logMsg)
{
    logFile << logMsg << "\n";
}


void vkb::Logger::log(const std::string& logMsg, const std::string& path)
{
    std::ofstream file = std::ofstream(path, std::ios_base::out);
    if (file.is_open())
        file << logMsg << "\n";
    else
        std::cout << "Unable to open file " << path << ".\n";
}
