#include "SceneEditorFileExplorer.h"



gui::FileContentsPreview::FileContentsPreview(fs::path filePath)
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

void gui::FileContentsPreview::operator()()
{
    // File preview popup
    if (!selectedFileName.empty())
    {
        // Begin transparent frame
        ig::PushStyleColor(ImGuiCol_TitleBg, ig::GetStyleColorVec4(ImGuiCol_WindowBg));
        ig::PushStyleColor(ImGuiCol_TitleBgActive, ig::GetStyleColorVec4(ImGuiCol_WindowBg));
        bool popupIsOpen{ true };
        ig::Begin(selectedFileName.c_str(), &popupIsOpen, ImGuiWindowFlags_NoCollapse);
        ig::PopStyleColor(2);

        ig::Separator();
        ig::Text("%s", selectedFileContent.c_str());

        ig::End();

        if (!popupIsOpen) {
            selectedFileName = "";
        }
    }
}



gui::FbxFileContextMenu::FbxFileContextMenu(fs::path path)
    :
    filePath(std::move(path)),
    initialPopupPos(vkb::Mouse::getPosition())
{
    assert(filePath.extension() == ".fbx");
}

void gui::FbxFileContextMenu::operator()()
{
    if (!isOpen) return;

    trc::imgui::WindowGuard guard;
    if (showImportDialog)
    {
        ig::Begin("FBX Import");
        importDialog.drawImGui();
    }
    else
    {
        ig::SetNextWindowPos({ initialPopupPos.x, initialPopupPos.y });
        util::beginContextMenuStyleWindow(filePath.c_str());
        if (ig::Button("Import"))
        {
            importDialog.loadFrom(filePath);
            showImportDialog = true;
        }
    }

    ig::Spacing();
    if (ig::Button("close")) {
        isOpen = false;
    }
}



gui::SceneEditorFileExplorer::SceneEditorFileExplorer()
    :
    fileExplorer("File Explorer", [this](const fs::path& selectedFile) {
        std::string ext = selectedFile.extension();
        auto it = contextMenuFuncs.find(ext);
        if (it != contextMenuFuncs.end())
        {
            currentContextFunc = it->second(selectedFile);
        }
        else
        {
            currentContextFunc = FileContentsPreview{ selectedFile };
        }
    }),
    contextMenuFuncs({
        { ".fbx", [](const fs::path& path) { return FbxFileContextMenu{ path }; } },
    })
{
}

void gui::SceneEditorFileExplorer::drawImGui()
{
    fileExplorer.drawImGui();
    currentContextFunc();
}
