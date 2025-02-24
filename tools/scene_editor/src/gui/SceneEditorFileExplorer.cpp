#include "SceneEditorFileExplorer.h"

#include <filesystem>
#include <fstream>
#include <sstream>

#include "Globals.h"
#include "gui/ContextMenu.h"
#include "gui/ImguiUtil.h"
#include "gui/ImportDialog.h"



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

void fileContextMenu(const fs::path& filePath)
{
    if (ig::Button("Preview")) {
        g::openFloatingWindow(FileContentsPreview(filePath));
    }

    if (ig::Button("Import"))
    {
        g::openFloatingWindow([importDialog = ImportDialog{filePath}]() mutable -> bool
        {
            gui::util::WindowGuard guard;
            if (!ig::Begin("Import")) {
                return false;
            }

            importDialog.drawImGui();
            return !ig::Button("Close");
        });
    }
}



SceneEditorFileExplorer::SceneEditorFileExplorer()
    :
    ImguiWindow("File Explorer"),
    fileExplorer([](const fs::path& selectedFile) {
        ContextMenu::show("Context", [selectedFile]{ fileContextMenu(selectedFile); });
    })
{
}

void SceneEditorFileExplorer::drawWindowContent()
{
    fileExplorer.drawImGui();
}

} // namespace gui
