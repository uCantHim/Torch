#pragma once

#include <filesystem>
namespace fs = std::filesystem;

#include "ImGuiUtil.h"

namespace gui
{
    /**
     * @brief Draw a file explorer inline in the current window
     *
     * @param const fs::path& currentPath The directory to show
     * @param auto onFileClick Callback for when a file or directory is
     *                         clicked on in the explorer. The path of the
     *                         clicked item is passed to the callback.
     */
    extern void fileExplorer(const fs::path& currentPath,
                             auto onFileClick,
                             bool showHiddenFiles = false);

    /**
     * @brief Class for a stand-alone file explorer window
     */
    class FileExplorer
    {
    public:
        FileExplorer(std::string title, std::function<void(const fs::path&)> fileClickCallback);

        void drawImGui();

    private:
        std::string title;
        std::function<void(const fs::path&)> fileClickCallback;

        // UI settings
        bool showHiddenFiles{ false };
    };
} // namespace gui
