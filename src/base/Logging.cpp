#include "trc/base/Logging.h"

#include <iostream>



namespace trc::log
{
    Logger<enableDebugLogging> debug(std::cout);
    Logger<enableDebugLogging> info(std::cout);
    Logger<enableDebugLogging> warn(std::cout);
    Logger<true> error(std::cout);
}
