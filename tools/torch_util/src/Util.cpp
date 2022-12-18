#include "trc_util/Util.h"

#include <fstream>
#include <sstream>
#include <stdexcept>



namespace trc::util
{

auto readFile(const fs::path& path) -> std::string
{
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("[In readFile]: Unable to open file " + path.string());
    }

    std::stringstream buf;
    buf << file.rdbuf();

    return buf.str();
}

} // namespace trc::util
