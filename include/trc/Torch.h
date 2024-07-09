#pragma once

#include <filesystem>

#include <trc_util/Timer.h>

#include "trc/AssetDescriptor.h"
#include "trc/Camera.h"
#include "trc/ShadowPool.h"
#include "trc/Types.h"
#include "trc/assets/Assets.h"
#include "trc/core/Frame.h"
#include "trc/core/Instance.h"
#include "trc/core/RenderConfiguration.h"
#include "trc/core/Renderer.h"
#include "trc/core/Window.h"
#include "trc/drawable/DrawableScene.h"

namespace trc
{
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

    using Scene = DrawableScene;

    struct TorchRenderConfigCreateInfo
    {
        // The asset registry from which the render config shall source asset
        // data. `assetDescriptor` must point to this asset registry.
        //
        // The created render config will apply updates to this asset registry's
        // device data during rendering.
        AssetRegistry& assetRegistry;

        // The instance that makes an AssetRegistry's data available to the
        // device. Create this descriptor, which contains information about
        // Torch's default assets, via `makeDefaultAssetModules`. This function
        // registers all asset modules that are necessary to use Torch's default
        // assets at an asset registry and builds a descriptor for their data.
        //
        // The same asset descriptor can be used for multiple render
        // configurations.
        //
        // The create render config will apply updates to this descriptor during
        // rendering.
        s_ptr<AssetDescriptor> assetDescriptor;

        // A pool from which shadow maps are allocated.
        s_ptr<ShadowPool> shadowDescriptor;

        // Decides whether the ray tracer is inserted into the rendering
        // pipeline.
        bool enableRayTracing{ true };

        // The maximum number of ray-traced geometry instances present in any
        // scene.
        ui32 maxRayGeometries{ 10000 };
    };

    auto makeTorchRenderPipeline(const Instance& instance,
                                 const Swapchain& swapchain,
                                 const TorchRenderConfigCreateInfo& createInfo)
        -> u_ptr<RenderPipeline>;

    /**
     * @brief Configuration for the `TorchStack` created by `initFull`
     */
    struct TorchStackCreateInfo
    {
        // TODO: Make these optional. Create in-memory asset storage if these
        // are not provided.
        fs::path projectRootDir{ TRC_COMPILE_ROOT_DIR"/torch_project_root" };
        fs::path assetStorageDir{ projectRootDir / "assets" };
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
        auto getShadowPool() -> ShadowPool&;
        auto getRenderPipeline() -> RenderPipeline&;

        /**
         * @brief Draw a frame
         *
         * Performs the required frame setup and -teardown tasks.
         */
        void drawFrame(Camera& camera, SceneBase& scene);

        void waitForAllFrames(ui64 timeoutNs = std::numeric_limits<ui64>::max());

    private:
        Instance instance;
        Window window;
        AssetManager assetManager;

        s_ptr<AssetDescriptor> assetDescriptor;
        s_ptr<ShadowPool> shadowPool;

        u_ptr<RenderPipeline> renderPipeline;

        Camera defaultCamera;
        SceneBase defaultScene;
        ViewportHandle viewport;

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
