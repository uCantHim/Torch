#pragma once

#include <concepts>
#include <functional>
#include <mutex>
#include <vector>

#include "trc/Types.h"
#include "trc/base/Buffer.h"
#include "trc/base/Memory.h"
#include "trc/core/DeviceTask.h"
#include "trc/core/RenderGraph.h"

namespace trc
{
    class DependencyRegion;
    class Device;
    class ResourceStorage;

    class FrameRenderState
    {
    public:
        /**
         * Register a callback that's called when all of the frame's commands
         * have been executed on a device.
         */
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

    class Frame : public FrameRenderState
    {
    public:
        Frame(const Device& device,
              RenderGraphLayout renderGraph,
              s_ptr<ResourceStorage> resources);

        ~Frame() noexcept = default;

        Frame(const Frame&) = delete;
        Frame(Frame&&) noexcept = delete;
        Frame& operator=(const Frame&) = delete;
        Frame& operator=(Frame&&) noexcept = delete;

        auto getDevice() const -> const Device&;
        auto getRenderGraph() const -> const RenderGraphLayout&;
        auto getResources() -> ResourceStorage&;
        auto getTaskQueue() -> DeviceTaskQueue&;

        auto makeTaskExecutionContext(s_ptr<DependencyRegion>) & -> DeviceExecutionContext;

        void spawnTask(RenderStage::ID stage, u_ptr<DeviceTask> task);

        template<std::invocable<vk::CommandBuffer, DeviceExecutionContext&> F>
        void spawnTask(RenderStage::ID stage, F&& taskFunc) {
            taskQueue.spawnTask(stage, std::forward<F>(taskFunc));
        }

        auto iterTasks(RenderStage::ID stage) {
            return taskQueue.iterTasks(stage);
        }

    private:
        const Device& device;
        RenderGraphLayout renderGraph;
        s_ptr<ResourceStorage> resources;

        DeviceTaskQueue taskQueue;
    };
} // namespace trc
