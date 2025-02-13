#pragma once

#include <filesystem>
#include <optional>
#include <string_view>

#include <trc_util/Exception.h>

#include "trc/assets/import/AssetImportBase.h"
#include "trc/text/Font.h"

namespace trc
{
    namespace fs = std::filesystem;

    class AssetRegistry;

    class DataImportError : public Exception
    {
    public:
        explicit DataImportError(std::string errorMsg) : Exception(std::move(errorMsg)) {}
    };

    /**
     * @brief Load all assets from a geometry file type
     *
     * Tries to interpret the file as a geometry or scene data file type,
     * such as COLLADA, FBX, OBJ, ...
     *
     * @throw DataImportError if the file format is not supported
     */
    auto loadAssets(const fs::path& filePath) -> ThirdPartyFileImportData;

    /**
     * @brief Load the first geometry from a file
     *
     * Tries to interpret the file as any of the geometry file types, such
     * as COLLADA, FBX, or OBJ.
     *
     * @throw DataImportError if the file format is not supported, or if the
     *                        file contains no geometries.
     */
    auto loadGeometry(const fs::path& filePath) -> GeometryData;

    /**
     * @brief Try to load a specific geometry from a file
     *
     * Tries to interpret the file as any of the geometry file types, such
     * as COLLADA, FBX, or OBJ.
     *
     * @return A geometry if one with the specified name exists. Nothing
     *         otherwise.
     * @throw DataImportError if the file format is not supported
     */
    auto loadGeometry(const fs::path& filePath, std::string_view name)
        -> std::optional<GeometryData>;

    /**
     * @brief Load a texture from an image file
     *
     * @throw DataImportError if an image cannot be loaded from `filePath`. This
     *                        might happen if the file format is not supported
     *                        or if the file cannot be opened.
     */
    auto loadTexture(const fs::path& filePath) -> TextureData;

    /**
     * @brief Load font data from any font file
     *
     * See the `FontData` struct for documentation on how to use the result.
     *
     * The operation is supported as long as Freetype supports the format of the
     * file at `path`.
     *
     * @throw DataImportError if `path` cannot be opened as a file in read
     *                           mode.
     */
    auto loadFont(const fs::path& path, ui32 fontSize) -> FontData;
} // namespace trc
