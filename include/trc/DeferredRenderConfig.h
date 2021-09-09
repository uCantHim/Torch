#pragma once

#include <vkb/event/Event.h>

#include "Types.h"
#include "core/Instance.h"
#include "core/RenderConfiguration.h"
#include "core/RenderPass.h"
#include "core/SceneBase.h"

#include "DescriptorProviderWrapper.h"
#include "RenderPassDeferred.h"
#include "RenderPassShadow.h"
#include "RenderDataDescriptor.h"
#include "SceneDescriptor.h"
#include "ShadowPool.h"
#include "AssetRegistry.h"
#include "AnimationDataStorage.h"
#include "text/FontDataStorage.h"

namespace trc
{
    class Window;
    class AssetRegistry;

    /**
     * @brief
     */
    struct DeferredRenderCreateInfo
    {
        AssetRegistry* assetRegistry;
        ShadowPool* shadowPool;

        ui32 maxTransparentFragsPerPixel{ 3 };
    };

    /**
     * @brief
     */
    class DeferredRenderConfig : public RenderConfigCrtpBase<DeferredRenderConfig>
    {
    public:
        /**
         * @brief
         */
        DeferredRenderConfig(const Window& window, const DeferredRenderCreateInfo& info);

        void preDraw(const DrawConfig& draw) override;
        void postDraw(const DrawConfig& draw) override;

        auto getDeferredRenderPass() const -> const RenderPassDeferred&;
        auto getCompatibleShadowRenderPass() const -> vk::RenderPass;

        auto getGlobalDataDescriptorProvider() const -> const DescriptorProviderInterface&;
        auto getSceneDescriptorProvider() const -> const DescriptorProviderInterface&;
        auto getDeferredPassDescriptorProvider() const -> const DescriptorProviderInterface&;
        auto getShadowDescriptorProvider() const -> const DescriptorProviderInterface&;
        auto getAssetDescriptorProvider() const -> const DescriptorProviderInterface&;
        auto getAnimationDataDescriptorProvider() const -> const DescriptorProviderInterface&;

        auto getAssets() -> AssetRegistry&;
        auto getAssets() const -> const AssetRegistry&;
        auto getShadowPool() -> ShadowPool&;
        auto getShadowPool() const -> const ShadowPool&;

    private:
        const Window& window;
        vkb::UniqueListenerId<vkb::SwapchainRecreateEvent> swapchainRecreateListener;

        // Default render passes
        u_ptr<RenderPassDeferred> deferredPass;
        RenderPassShadow shadowPass;

        // Descriptors
        GlobalRenderDataDescriptor globalDataDescriptor;
        SceneDescriptor sceneDescriptor;

        /**
         * Use a wrapper here because the pass is recreated on swapchain
         * resize. This way I don't have to recreate the pipelines.
         */
        DescriptorProviderWrapper deferredPassDescriptorProvider;

        // Data & Assets
        AssetRegistry* assetRegistry;
        ShadowPool* shadowPool;

        // Final lighting pass stuff
        vkb::DeviceLocalBuffer fullscreenQuadVertexBuffer;
        DrawableExecutionRegistration::ID finalLightingFunc;
    };
} // namespace trc
