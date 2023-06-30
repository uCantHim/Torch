#pragma once

#include "Scene.h"

namespace g
{
    auto scene() -> Scene&;
    auto torch() -> trc::TorchStack&;
} // namespace g
