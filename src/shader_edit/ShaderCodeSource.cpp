#include "shader_edit/ShaderCodeSource.h"

#include <sstream>
#include <fstream>



shader_edit::FileSource::FileSource(const fs::path& filePath)
    :
    filePath(filePath)
{
    if (!fs::is_regular_file(filePath))
    {
        throw std::invalid_argument(
            "[In FileSource::FileSource]: File \"" + filePath.string()
            + "\" is not a regular file!"
        );
    }
}

auto shader_edit::FileSource::getCode() const -> std::string
{
    std::stringstream ss;
    ss << std::ifstream(filePath).rdbuf();

    return ss.str();
}



shader_edit::StringSource::StringSource(std::string str)
    :
    str(std::move(str))
{
}

auto shader_edit::StringSource::getCode() const -> std::string
{
    return str;
}
