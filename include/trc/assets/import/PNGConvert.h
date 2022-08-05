#pragma once

#include <vector>

#include "trc/Types.h"
#include "trc/assets/TextureRegistry.h"

namespace trc
{
    /**
     * @brief Test if a file contains PNG-formatted data
     *
     * Reads the first eight bytes from the file.
     *
     * @return bool True if the file contains PNG data
     */
    bool isPNG(const fs::path& file);

    /**
     * @brief Test if data is PNG-formatted
     *
     * @return bool True if the data is valid PNG data
     */
    bool isPNG(const void* data, size_t size);

    auto toPNG(const TextureData& tex) -> std::vector<ui8>;
    auto fromPNG(const std::vector<ui8>& data) -> TextureData;
    auto fromPNG(const void* data, size_t size) -> TextureData;
} // namespace trc::util
