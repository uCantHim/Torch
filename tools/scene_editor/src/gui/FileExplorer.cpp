#include "FileExplorer.h"




namespace gui
{

/**
 * @brief Push a style color according to the type of filesystem entry
 */
inline void setTextColor(const fs::directory_entry& entry)
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
    for (int id = 0; const auto& entry : dirIt)
    {
        const auto entryName = entry.path().filename();
        if (!showHiddenFiles && entryName.c_str()[0] == '.') {
            continue;
        }

        ig::PushID(id++);
        setTextColor(entry);

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



FileExplorer::FileExplorer(std::function<void(const fs::path&)> fileClickCallback)
    :
    fileClickCallback(std::move(fileClickCallback))
{
}

void FileExplorer::drawImGui()
{
    static std::array<char, 4096> directoryBuf{ [] {
        std::array<char, 4096> result;
        auto currentPath{ fs::current_path().string() };
        memcpy(result.data(), currentPath.data(), currentPath.size());
        return result;
    }()};

    ig::PushItemWidth(200);
    ig::InputText("Enter a directory path", directoryBuf.data(), 4096);
    // Function to set the current directory manually
    auto setCurrentDirectory = [](const std::string& buf) {
        memcpy(directoryBuf.data(), buf.data(), buf.size());
        directoryBuf[buf.size()] = '\0';
    };

    ig::Checkbox("Show hidden files", &showHiddenFiles);

    fs::path currentDir{ directoryBuf.data() };
    // Expand tilde on current dir
    if (currentDir.c_str()[0] == '~') {
        currentDir = std::string(getenv("HOME")) + currentDir.string().substr(1);
    }

    if (!fs::is_directory(currentDir))
    {
        ig::PushStyleColor(ImGuiCol_Text, { 1, 0, 0, 1 });
        ig::Text("%s is not a directory.", directoryBuf.data());
        ig::PopStyleColor();
        return;
    }

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
