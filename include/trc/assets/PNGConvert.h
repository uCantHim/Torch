#pragma once

#include <vector>

#include "RawData.h"

namespace trc
{
    auto toPNG(const TextureData& tex) -> std::vector<ui8>;
    auto fromPNG(const std::vector<ui8>& data) -> TextureData;
    auto fromPNG(const void* data, size_t size) -> TextureData;
} // namespace trc::util
