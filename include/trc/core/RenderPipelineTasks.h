#pragma once

#include "trc/core/DeviceTask.h"
#include "trc/core/RenderPluginContext.h"
#include "trc/core/TaskQueue.h"

namespace trc
{
    class AssetManager;
    class Camera;
    class RenderPipeline;
    class ResourceStorage;
    class SceneBase;

    /**
     * Data that can be accessed while dispatching tasks.
     */
    struct GlobalUpdateContext : DeviceExecutionContext
                               , impl::RenderPipelineInfo
    {
        GlobalUpdateContext(const DeviceExecutionContext& devCtx,
                            const impl::RenderPipelineInfo& info)
            : DeviceExecutionContext(devCtx), impl::RenderPipelineInfo(info) {}
    };

    /**
     * Data that can be accessed while dispatching scene-specific tasks.
     */
    struct SceneUpdateContext : DeviceExecutionContext
                              , impl::SceneInfo
    {
        SceneUpdateContext(const DeviceExecutionContext& devCtx,
                           const impl::SceneInfo& info)
            : DeviceExecutionContext(devCtx), impl::SceneInfo(info) {}
    };

    /**
     * Data that can be accessed while dispatching viewport-specific tasks.
     */
    struct ViewportDrawContext : DeviceExecutionContext
                               , impl::ViewportInfo
    {
        ViewportDrawContext(const DeviceExecutionContext& devCtx,
                            const impl::ViewportInfo& vpInfo)
            : DeviceExecutionContext(devCtx), impl::ViewportInfo(vpInfo) {}
    };

    using GlobalUpdateTask = impl::Task<GlobalUpdateContext>;
    using SceneUpdateTask = impl::Task<SceneUpdateContext>;
    /**
     * Defines draw operations on per-viewport data.
     */
    using ViewportDrawTask = impl::Task<ViewportDrawContext>;

    using GlobalUpdateTaskQueue = impl::TaskQueue<GlobalUpdateContext>;
    using SceneUpdateTaskQueue = impl::TaskQueue<SceneUpdateContext>;
    using ViewportDrawTaskQueue = impl::TaskQueue<ViewportDrawContext>;
} // namespace trc
