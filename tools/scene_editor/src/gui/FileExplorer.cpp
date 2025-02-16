#include "FileExplorer.h"

#include <algorithm>
#include <ranges>

#include <imgui.h>
namespace ig = ImGui;



namespace gui
{

/**
 * @brief Push a style color according to the type of filesystem entry
 */
void pushTextColor(const fs::directory_entry& entry)
{
    // Check symlink first because directories may also be symlinks
    if (entry.is_symlink()) {
        ig::PushStyleColor(ImGuiCol_Text, { 1.0, 0.6, 0.0, 1.0 });
    }
    else if (entry.is_directory()) {
        ig::PushStyleColor(ImGuiCol_Text, { 0, 0.4, 1.0, 1.0 });
    }
    else if (entry.is_regular_file()) {
        ig::PushStyleColor(ImGuiCol_Text, { 1.0, 1.0, 1.0, 1.0 });
    }
    else {
        ig::PushStyleColor(ImGuiCol_Text, { 0.0, 1.0, 0.0, 1.0 });
    }
}

void fileExplorer(const fs::path& currentPath, auto onFileClick, bool showHiddenFiles)
{
    fs::directory_iterator dirIt(currentPath);
    for (const auto& [id, entry] : std::views::enumerate(dirIt))
    {
        const auto entryName = entry.path().filename();
        if (!showHiddenFiles && entryName.c_str()[0] == '.') {
            continue;
        }

        ig::PushID(id);
        pushTextColor(entry);
        if (ig::Selectable("", false)) {
            onFileClick(entry.path());
        }
        ig::SameLine();
        ig::Text("%s", entryName.c_str());
        ig::PopStyleColor();

        if (entry.is_symlink())
        {
            ig::SameLine();
            ig::Text("-> %s", fs::read_symlink(entry.path()).c_str());
        }

        ig::PopID();
    }
}



FileExplorer::FileExplorer(
    std::function<void(const fs::path&)> fileClickCallback,
    const fs::path& initialDirectory)
    :
    fileClickCallback(std::move(fileClickCallback))
{
    const auto str = initialDirectory.string();
    directoryBuf.resize(std::max(4096ul, str.size()));
    memcpy(directoryBuf.data(), str.data(), str.size());
}

void FileExplorer::drawImGui()
{
    // Function to set the current directory manually
    auto setCurrentDirectory = [this](const std::string& str) {
        if (directoryBuf.size() <= str.size()) {
            directoryBuf.resize(str.size() + 1);
        }
        memcpy(directoryBuf.data(), str.data(), str.size());
        directoryBuf[str.size()] = '\0';
    };

    // Settings
    ig::PushItemWidth(200);
    ig::InputText("Enter a directory path", directoryBuf.data(), 4096);
    ig::Checkbox("Show hidden files", &showHiddenFiles);

    // Expand tilde to home directory on the current path
    fs::path currentDir{ directoryBuf.data() };
    if (!currentDir.empty() && currentDir.c_str()[0] == '~') {
        currentDir = std::string(getenv("HOME")) + currentDir.string().substr(1);
    }

    if (!fs::is_directory(currentDir))
    {
        ig::PushStyleColor(ImGuiCol_Text, { 1, 0, 0, 1 });
        ig::Text("%s is not a directory.", currentDir.c_str());
        ig::PopStyleColor();
        return;
    }

    // Show the current directory and a 'move to previous directory' button
    ig::Separator();
    if (ig::Button("back") && currentDir.has_parent_path()) {
        setCurrentDirectory(currentDir.parent_path().string());
    }
    ig::SameLine();
    ig::Text("%s", currentDir.c_str());
    ig::Separator();

    // Draw the actual file explorer
    fileExplorer(currentDir, [&](const fs::path& file) {
        if (fs::is_directory(file)) {
            setCurrentDirectory(file.string());
        }
        else if (fs::is_regular_file(file)) {
            fileClickCallback(file);
        }
    }, showHiddenFiles);
}

} // namespace gui
