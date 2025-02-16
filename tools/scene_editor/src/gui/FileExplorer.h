#pragma once

#include <filesystem>
#include <functional>
#include <vector>
namespace fs = std::filesystem;

namespace gui
{
    /**
     * @brief Class for a stand-alone file explorer window
     */
    class FileExplorer
    {
    public:
        explicit FileExplorer(std::function<void(const fs::path&)> fileClickCallback,
                              const fs::path& initialDirectory = fs::current_path());

        void drawImGui();

    private:
        std::function<void(const fs::path&)> fileClickCallback;
        std::vector<char> directoryBuf;

        // UI settings
        bool showHiddenFiles{ false };
    };
} // namespace gui
