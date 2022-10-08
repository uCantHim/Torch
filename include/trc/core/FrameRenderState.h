#pragma once

#include <vector>
#include <functional>
#include <mutex>

#include "trc/base/Buffer.h"
#include "trc/base/Device.h"

#include "trc/Types.h"

namespace trc
{
    class FrameRenderState
    {
    public:
        void onRenderFinished(std::function<void()> func);

        /**
         * @brief Create a temporary buffer that lives for the duration of
         *        one frame
         */
        auto makeTransientBuffer(const Device& device,
                                 size_t size,
                                 vk::BufferUsageFlags usageFlags,
                                 vk::MemoryPropertyFlags memoryFlags,
                                 const DeviceMemoryAllocator& alloc
                                     = DefaultDeviceMemoryAllocator{})
            -> Buffer&;

    private:
        friend class Renderer;
        void signalRenderFinished();

        std::mutex mutex;
        std::vector<std::function<void()>> renderFinishedCallbacks;
        std::vector<u_ptr<Buffer>> transientBuffers;
    };
} // namespace trc
