#pragma once

#include <sstream>
#include <fstream>
#include <filesystem>
namespace fs = std::filesystem;

namespace shader_edit
{
    inline auto render(const std::ifstream& file) -> std::string
    {
        std::stringstream ss;
        ss << file.rdbuf();
        return ss.str();
    }

    inline auto render(const fs::path& path) -> std::string
    {
        if (!fs::is_regular_file(path))
        {
            throw std::invalid_argument(
                "[In render(fs::path&)]: File " + path.string() + " is not a regular file."
            );
        }

        return render(std::ifstream(path));
    }
} // namespace shader_edit
