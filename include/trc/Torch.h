#pragma once

#include <filesystem>
#include <optional>

#include "trc/AssetDescriptor.h"
#include "trc/Camera.h"
#include "trc/Types.h"
#include "trc/assets/Assets.h"
#include "trc/core/Frame.h"
#include "trc/core/Instance.h"
#include "trc/core/RenderPipeline.h"
#include "trc/core/Renderer.h"
#include "trc/core/Window.h"
#include "trc/drawable/DrawableScene.h"

namespace trc
{
    using Scene = DrawableScene;

    /**
     */
    struct TorchInitInfo
    {
        bool startEventThread{ true };
    };

    /**
     * @brief Initialize Torch globally
     *
     * Required to be called once before any other Torch functionality can
     * be used. Calls to `init` before `terminate` has been called return
     * immediately.
     *
     * Call `terminate` to de-initialize Torch.
     *
     * Use `initFull` instead to initialize a complete default configuration
     * of Torch with all required objects and services for rendering.
     */
    void init(const TorchInitInfo& info = {});

    /**
     * @brief Poll system events
     */
    void pollEvents();

    /**
     * @brief Destroy all resources allocated by Torch
     *
     * You should release all of your resources before calling this
     * function.
     */
    void terminate();

    struct TorchPipelineCreateInfo
    {
        // The maximum number of viewports that may be allocated from the
        // resulting pipeline. Must be greater than 0.
        ui32 maxViewports{ 1 };

        // The asset registry that is being updated by the pipeline. Access an
        // asset manager's registry with `AssetManager::getDeviceRegistry`.
        AssetRegistry& assetRegistry;

        AssetDescriptorCreateInfo assetDescriptorCreateInfo;

        // The maximum number of shadow maps supported by the pipeline.
        ui32 maxShadowMaps{ 100 };

        // A heuristic used to allocate pre-sized fragment list buffers.
        ui32 maxTransparentFragsPerPixel{ 3 };

        // Decides whether the ray tracer is inserted into the rendering
        // pipeline.
        bool enableRayTracing{ true };

        // The maximum number of ray-traced geometry instances present in any
        // scene.
        ui32 maxRayGeometries{ 10000 };
    };

    using TorchPipelinePluginBuilder = std::function<PluginBuilder(Window&)>;

    auto makeTorchRenderPipeline(Instance& instance,
                                 Window& window,
                                 const TorchPipelineCreateInfo& createInfo,
                                 const vk::ArrayProxy<TorchPipelinePluginBuilder>& plugins = {})
        -> u_ptr<RenderPipeline>;

    /**
     * @brief Configuration for the `TorchStack` created by `initFull`
     */
    struct TorchStackCreateInfo
    {
        /**
         * @brief Specify a list of additional render plugins that shall be
         *        inserted into Torch's render pipeline.
         */
        vk::ArrayProxy<TorchPipelinePluginBuilder> plugins;

        /**
         * @brief A path to the project's asset storage directory.
         *
         * If not given, no asset storage is used. Instead, assets can be
         * created in-memory and referenced by their assigned IDs.
         */
        std::optional<fs::path> assetStorageDir;
    };

    /**
     * @brief A collection of objects required to render stuff
     *
     * It is certainly possible to create all of these yourself. This is
     * a convenient default implementation.
     */
    struct TorchStack
    {
        TorchStack(const TorchStack&) = delete;
        TorchStack(TorchStack&&) noexcept = delete;
        auto operator=(const TorchStack&) -> TorchStack& = delete;
        auto operator=(TorchStack&&) noexcept -> TorchStack& = delete;

        TorchStack(const TorchStackCreateInfo& torchConfig = {},
                   const InstanceCreateInfo& instanceInfo = {},
                   const WindowCreateInfo& windowInfo = {},
                   const AssetDescriptorCreateInfo& assetDescriptorInfo = {});
        ~TorchStack();

        auto getDevice() -> Device&;
        auto getInstance() -> Instance&;
        auto getWindow() -> Window&;
        auto getAssetManager() -> AssetManager&;
        auto getRenderPipeline() -> RenderPipeline&;

        /**
         * @throw std::out_of_range if more viewports are allocated than the
         *                          allowed maximum. (default: 1)
         */
        auto makeViewport(const RenderArea& area,
                          const s_ptr<Camera>& camera,
                          const s_ptr<SceneBase>& scene)
            -> ViewportHandle;

        /**
         * Create a viewport that always resizes to the full window size.
         */
        auto makeFullscreenViewport(const s_ptr<Camera>& camera,
                                    const s_ptr<SceneBase>& scene)
            -> ViewportHandle;

        /**
         * @brief Draw a frame
         */
        void drawFrame(const vk::ArrayProxy<ViewportHandle>& viewports);

        void waitForAllFrames(ui64 timeoutNs = std::numeric_limits<ui64>::max());

    private:
        static constexpr ui32 kDefaultMaxShadowMaps{ 200 };
        static constexpr ui32 kDefaultMaxTransparentFrags{ 3 };

        Instance instance;
        Window window;
        AssetManager assetManager;

        u_ptr<RenderPipeline> renderPipeline;

        Renderer renderer;
    };

    /**
     * @brief Create a full default configuration of Torch
     */
    auto initFull(const TorchStackCreateInfo& torchConfig = {},
                  const InstanceCreateInfo& instanceInfo = {},
                  const WindowCreateInfo& windowInfo = {}
                  ) -> u_ptr<TorchStack>;
} // namespace trc
