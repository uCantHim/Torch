#pragma once

#include "trc/GBuffer.h"
#include "trc/UpdatePass.h"
#include "trc/base/Buffer.h"
#include "trc/base/FrameSpecificObject.h"

namespace trc
{
    class GBufferDepthReader : public UpdatePass
    {
    public:
        GBufferDepthReader(const Device& device,
                           std::function<vec2()> mousePosGetter,
                           GBuffer& gBuffer);

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

        GBuffer& gBuffer;
        Buffer depthPixelReadBuffer;
        ui32* depthBufMap{ depthPixelReadBuffer.map<ui32*>() };
    };
} // namespace trc
