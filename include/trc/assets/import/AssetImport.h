#pragma once

#include <expected>
#include <filesystem>
#include <string_view>

#include <trc_util/Exception.h>

#include "trc/assets/import/AssetImportBase.h"
#include "trc/text/Font.h"

namespace trc
{
    namespace fs = std::filesystem;

    /**
     * @brief Load all assets from a geometry file type
     *
     * Tries to interpret the file as a geometry or scene data file type,
     * such as COLLADA, FBX, OBJ, ...
     *
     * @throw DataImportError if the file format is not supported
     */
    auto importAssets(const fs::path& filePath)
        -> std::expected<import::ThirdPartyImport, import::ImportError>;

    /**
     * @brief Load the first geometry from a file
     *
     * Tries to interpret the file as any of the geometry file types, such
     * as COLLADA, FBX, or OBJ.
     *
     * @throw DataImportError if the file format is not supported, or if the
     *                        file contains no geometries.
     */
    auto importGeometry(const fs::path& filePath)
        -> std::expected<GeometryData, import::ImportError>;

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
    auto importGeometry(const fs::path& filePath, std::string_view name)
        -> std::expected<GeometryData, import::ImportError>;

    /**
     * @brief Load a texture from an image file
     *
     * @throw DataImportError if an image cannot be loaded from `filePath`. This
     *                        might happen if the file format is not supported
     *                        or if the file cannot be opened.
     */
    auto importTexture(const fs::path& filePath)
        -> std::expected<TextureData, import::ImportError>;

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
    auto importFont(const fs::path& path, ui32 fontSize)
        -> std::expected<FontData, import::ImportError>;
} // namespace trc
