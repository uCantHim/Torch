#include "SceneEditorFileExplorer.h"

#include <filesystem>
#include <fstream>
#include <sstream>

#include "MainMenu.h"
#include "ContextMenu.h"
#include "ImportDialog.h"

namespace fs = std::filesystem;



namespace gui
{

/**
 * A preview window for text file content.
 */
struct FileContentsPreview
{
    explicit FileContentsPreview(fs::path filePath)
    {
        // Default callback for unknown file type
        std::ifstream file(filePath);
        if (file.is_open())
        {
            std::stringstream ss;
            ss << file.rdbuf();
            selectedFileName = filePath;
            selectedFileContent = ss.str();
        }
        else {
            selectedFileName = "";
        }
    }

    bool operator()()
    {
        // File preview popup
        if (selectedFileName.empty()) {
            return false;
        }

        // Begin transparent frame
        ig::PushStyleColor(ImGuiCol_TitleBg, ig::GetStyleColorVec4(ImGuiCol_WindowBg));
        ig::PushStyleColor(ImGuiCol_TitleBgActive, ig::GetStyleColorVec4(ImGuiCol_WindowBg));
        bool popupIsOpen{ true };
        ig::Begin(selectedFileName.c_str(), &popupIsOpen, ImGuiWindowFlags_NoCollapse);
        ig::PopStyleColor(2);

        // Show content
        ig::Separator();
        ig::Text("%s", selectedFileContent.c_str());

        // End
        ig::End();

        return popupIsOpen;
    }

    fs::path selectedFileName;
    std::string selectedFileContent;
};

/**
 * Context menu for filesystem entries
 */
class FileContextMenu
{
public:
    FileContextMenu(MainMenu& menu, fs::path path)
        :
        menu(menu),
        filePath(std::move(path))
    {}

    void operator()()
    {
        if (ig::Button("Preview")) {
            menu.openWindow(FileContentsPreview(filePath));
        }

        if (ig::Button("Import"))
        {
            menu.openWindow([importDialog = ImportDialog(filePath)]() mutable -> bool
            {
                trc::experimental::imgui::WindowGuard guard;
                if (!ig::Begin("Import")) {
                    return false;
                }

                importDialog.drawImGui();
                if (ig::Button("Close")) {
                    return false;
                }
                return true;
            });
        }
    }

private:
    MainMenu& menu;
    fs::path filePath;
};



SceneEditorFileExplorer::SceneEditorFileExplorer(MainMenu& menu)
    :
    fileExplorer("File Explorer", [&menu](const fs::path& selectedFile) {
        ContextMenu::show("Context", FileContextMenu(menu, selectedFile));
    })
{
}

void SceneEditorFileExplorer::drawImGui()
{
    fileExplorer.drawImGui();
}

} // namespace gui
