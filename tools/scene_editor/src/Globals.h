#pragma once

#include "Scene.h"
#include "asset/AssetInventory.h"

namespace g
{
    auto assets() -> AssetInventory&;
    auto scene() -> Scene&;
    auto torch() -> trc::TorchStack&;
} // namespace g
