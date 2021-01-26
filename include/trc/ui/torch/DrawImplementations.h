#pragma once

#include "ui/DrawInfo.h"

namespace trc::internal
{
    extern void initGuiDraw(vk::RenderPass renderPass);
    extern void cleanupGuiDraw();

    extern void drawElement(const ui::DrawInfo& info, vk::CommandBuffer cmdBuf);
} // namespace trc::internal
