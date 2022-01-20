#pragma once

#include <string>
#include <unordered_map>

#include "ImportDialog.h"
#include "FileExplorer.h"

namespace gui
{
    struct FileContentsPreview
    {
        explicit FileContentsPreview(fs::path filePath);
        void operator()();

        fs::path selectedFileName;
        std::string selectedFileContent;
    };

    struct FbxFileContextMenu
    {
        explicit FbxFileContextMenu(fs::path filePath);
        void operator()();

        fs::path filePath;
        bool isOpen{ true };
        bool showImportDialog{ false };

        vec2 initialPopupPos;
        FbxImportDialog importDialog;
    };

    class SceneEditorFileExplorer
    {
    public:
        SceneEditorFileExplorer();

        void drawImGui();

    private:
        gui::FileExplorer fileExplorer;

        using ContextFuncCreateFunc = std::function<std::function<void()>(const fs::path&)>;
        std::unordered_map<std::string, ContextFuncCreateFunc> contextMenuFuncs;
        std::function<void()> currentContextFunc{ []{} };
    };
} // namespace gui
