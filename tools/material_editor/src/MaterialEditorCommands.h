#pragma once

#include <filesystem>
namespace fs = std::filesystem;

#include "GraphManipulator.h"

class MaterialPreview;

/**
 * @brief Interface that provides actions which are invoked by user input
 */
class MaterialEditorCommands : public GraphManipulator
{
public:
    MaterialEditorCommands(GraphScene& graph, MaterialPreview& preview);

    /**
     * @brief Serialize the material graph and write it to the currently
     *        edited save file.
     */
    void saveGraphToCurrentFile();

    /**
     * @brief Serialize the material graph and write it to a file
     *
     * @param fs::path path A file path to which the graph is exported.
     */
    void saveGraphToFile(const fs::path& path);

    /**
     * @brief Compile the graph to a material
     *
     * Shows the material in the preview window, if compilation was successful.
     */
    void compileMaterial();

private:
    GraphScene& graph;
    MaterialPreview& previewWindow;
};
