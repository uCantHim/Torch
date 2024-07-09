#pragma once

#include "trc/Types.h"
#include "trc/core/DataFlow.h"
#include "trc/core/TaskQueue.h"

namespace trc
{
    class Device;
    class Frame;
    class ResourceStorage;

    /**
     * @brief Context in which a device task is executed.
     */
    struct DeviceExecutionContext
    {
        /**
         * Prefer to call `Frame::makeTaskExecutionContext` instead.
         */
        DeviceExecutionContext(Frame& _frame,
                               const s_ptr<DependencyRegion>& depRegion,
                               const s_ptr<ResourceStorage>& resources);

        auto frame() -> Frame&;
        auto device() -> const Device&;

        auto resources() -> ResourceStorage&;
        auto deps() -> DependencyRegion&;

        // TODO?: auto makeTransientBuffer(vk::DeviceSize size) -> Buffer&;

        auto overrideResources(s_ptr<ResourceStorage> newStorage) const -> DeviceExecutionContext;

    private:
        Frame& parentFrame;
        s_ptr<DependencyRegion> dependencyRegion;
        s_ptr<ResourceStorage> resourceStorage;  // May not be the same as the frame's.
    };

    /**
     * @brief A generic task that records device commands.
     */
    using DeviceTask = impl::Task<DeviceExecutionContext>;

    /**
     * @brief A task queue for generic device tasks.
     */
    using DeviceTaskQueue = impl::TaskQueue<DeviceExecutionContext>;
} // namespace trc
