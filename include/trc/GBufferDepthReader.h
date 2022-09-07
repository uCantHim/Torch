#pragma once

#include <vkb/Buffer.h>
#include <vkb/FrameSpecificObject.h>

#include "GBuffer.h"
#include "UpdatePass.h"

namespace trc
{
    class Viewport;
    class Window;

    class GBufferDepthReader : public UpdatePass
    {
    public:
        GBufferDepthReader(const vkb::Device& device,
                           std::function<vec2()> mousePosGetter,
                           vkb::FrameSpecific<GBuffer>& gBuffer);

        void update(vk::CommandBuffer cmdBuf, FrameRenderState&) override;

        /**
         * @brief Get the depth of pixel under the mouse cursor
         *
         * @return float Depth of the pixel which contains the mouse cursor.
         *               Is the last read depth value if the cursor is not
         *               in a window.
         */
        auto getMouseDepth() const noexcept -> float;

    private:
        void readDepthAtMousePos(vk::CommandBuffer cmdBuf);

        std::function<vec2()> getMousePos;

        vkb::FrameSpecific<GBuffer>& gBuffer;
        vkb::Buffer depthPixelReadBuffer;
        ui32* depthBufMap{ depthPixelReadBuffer.map<ui32*>() };
    };
} // namespace trc
