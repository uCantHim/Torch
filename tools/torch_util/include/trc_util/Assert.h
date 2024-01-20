#pragma once

#include <stdexcept>
#include <source_location>
#include <string>

/**
 * @brief Verify an assumption about a function argument
 *
 * This is not a 'true' assertion as it is not disabled in release mode. It is
 * intended to be used as part of a function's interface's implementation.
 *
 * @throw std::invalid_argument if the assertion fails.
 */
#define assert_arg(expr) \
    if (!(expr)) { \
        constexpr auto loc = std::source_location::current(); \
        throw std::invalid_argument( \
            "[In " \
            + std::string{loc.file_name()} + ":" + std::to_string(loc.line()) \
            + " | " + std::string{loc.function_name()} \
            + "] Assertion failed! Function argument must satisfy " #expr \
        ); \
    }
