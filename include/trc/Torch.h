#pragma once

#include <filesystem>

#include <trc_util/Timer.h>

#include "trc/Camera.h"
#include "trc/Scene.h"
#include "trc/TorchRenderConfig.h"
#include "trc/TorchRenderStages.h"
#include "trc/Types.h"
#include "trc/assets/Assets.h"
#include "trc/core/DrawConfiguration.h"
#include "trc/core/Instance.h"
#include "trc/core/RenderTarget.h"
#include "trc/core/Window.h"
#include "trc/drawable/Drawable.h"

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
                   const WindowCreateInfo& windowInfo = {});
        ~TorchStack();

        auto getDevice() -> Device&;
        auto getInstance() -> Instance&;
        auto getWindow() -> Window&;
        auto getAssetManager() -> AssetManager&;
        auto getShadowPool() -> ShadowPool&;
        auto getRenderTarget() -> RenderTarget&;
        auto getRenderConfig() -> TorchRenderConfig&;

        /**
         * @brief Draw a frame
         *
         * Performs the required frame setup and -teardown tasks.
         */
        void drawFrame(const Camera& camera, const Scene& scene);

    private:
        Instance instance;
        Window window;
        AssetManager assetManager;
        RenderTarget swapchainRenderTarget;
        TorchRenderConfig renderConfig;

        UniqueListenerId<SwapchainRecreateEvent> swapchainRecreateListener;
    };

    /**
     * @brief Create a full default configuration of Torch
     */
    auto initFull(const TorchStackCreateInfo& torchConfig = {},
                  const InstanceCreateInfo& instanceInfo = {},
                  const WindowCreateInfo& windowInfo = {}
                  ) -> u_ptr<TorchStack>;
} // namespace trc
