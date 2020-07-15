#pragma once

#include <string>
#include <exception>

namespace trc
{
    class Exception : public std::exception
    {
    public:
        explicit Exception(std::string errorMsg = "")
            : errorMsg(std::move(errorMsg))
        {}

        auto what() const noexcept -> const char* override {
            return errorMsg.c_str();
        }

    private:
        std::string errorMsg;
    };
}
