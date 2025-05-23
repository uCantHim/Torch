#include "util.h"

#include <fstream>



std::ofstream logFile;
trc::Logger<trc::log::LogLevel::eDebug, _enableLogging> debug{
    [] -> std::ostream& {
        if (_enableLogging)
        {
            logFile.open("cloth-lsp.log", std::ios_base::app);
            return logFile;
        }
        return std::cerr;
    }()
};
