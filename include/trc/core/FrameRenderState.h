#pragma once

#include <vector>
#include <functional>
#include <mutex>

#include <vkb/Buffer.h>
#include <vkb/Device.h>

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
        auto makeTransientBuffer(const vkb::Device& device,
                                 size_t size,
                                 vk::BufferUsageFlags usageFlags,
                                 vk::MemoryPropertyFlags memoryFlags,
                                 const vkb::DeviceMemoryAllocator& alloc
                                     = vkb::DefaultDeviceMemoryAllocator{})
            -> vkb::Buffer&;

    private:
        friend class Renderer;
        void signalRenderFinished();

        std::mutex mutex;
        std::vector<std::function<void()>> renderFinishedCallbacks;
        std::vector<u_ptr<vkb::Buffer>> transientBuffers;
    };
} // namespace trc
