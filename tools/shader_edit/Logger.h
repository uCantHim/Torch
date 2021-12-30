#pragma once

#include <string>
#include <iostream>

namespace shader_edit
{
    inline bool verboseLogging{ false };

    inline void info(const std::string& info)
    {
        if (verboseLogging) {
            std::cout << "[Info] " << info << "\n";
        }
    }

    inline void warn(const std::string& warning)
    {
        std::cout << "[Warning] " << warning << "\n";
    }

    inline void error(const std::string& error)
    {
        std::cout << "[Error] " << error << "\n";
    }
} // namespace shader_edit
