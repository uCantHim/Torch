#pragma once

#include "trc/AssetDescriptor.h"
#include "trc/FinalLightingPass.h"
#include "trc/GBufferDepthReader.h"
#include "trc/GBufferPass.h"
#include "trc/RenderDataDescriptor.h"
#include "trc/RenderPassShadow.h"
#include "trc/SceneDescriptor.h"
#include "trc/ShadowPool.h"
#include "trc/TopLevelAccelerationStructureBuildPass.h"
#include "trc/Types.h"
#include "trc/assets/AssetRegistry.h"
#include "trc/base/event/Event.h"
#include "trc/core/DescriptorProviderWrapper.h"
#include "trc/core/Instance.h"
#include "trc/core/RenderConfigImplHelper.h"
#include "trc/core/RenderGraph.h"
#include "trc/core/RenderPass.h"
#include "trc/core/SceneBase.h"
#include "trc/ray_tracing/AccelerationStructure.h"
#include "trc/ray_tracing/RayTracingPass.h"

namespace trc
{
    class Window;

    /**
     * @brief
     */
    struct TorchRenderConfigCreateInfo
    {
        const RenderTarget& target;

        AssetRegistry* assetRegistry;
        s_ptr<AssetDescriptor> assetDescriptor;
        ShadowPool* shadowPool;

        ui32 maxTransparentFragsPerPixel{ 3 };
        bool enableRayTracing{ false };
    };

    auto makeTorchRenderGraph() -> RenderGraph;

    /**
     * @brief
     */
    class TorchRenderConfig : public RenderConfigImplHelper
    {
    public:
        /**
         * Camera matrices, resolution, mouse position
         */
        static constexpr auto GLOBAL_DATA_DESCRIPTOR{ "global_data" };

        /**
         * All of the asset registry's data
         */
        static constexpr auto ASSET_DESCRIPTOR{ "asset_registry" };

        /**
         * Lights
         */
        static constexpr auto SCENE_DESCRIPTOR{ "scene_data" };

        /**
         * Storage images, transparency buffer, swapchain image
         */
        static constexpr auto G_BUFFER_DESCRIPTOR{ "g_buffer" };

        /**
         * Shadow matrices, shadow maps
         */
        static constexpr auto SHADOW_DESCRIPTOR{ "shadow" };

        static constexpr auto OPAQUE_G_BUFFER_PASS{ "g_buffer" };
        static constexpr auto TRANSPARENT_G_BUFFER_PASS{ "transparency" };
        static constexpr auto SHADOW_PASS{ "shadow" };
        static constexpr auto FINAL_LIGHTING_PASS{ "final_lighting" };

        /**
         * @brief
         */
        TorchRenderConfig(const Window& window, const TorchRenderConfigCreateInfo& info);

        void perFrameUpdate(const Camera& camera, const Scene& scene);

        void setViewport(uvec2 offset, uvec2 size);
        void setRenderTarget(const RenderTarget& newTarget);

        void setClearColor(vec4 color);

        auto getGBuffer() -> FrameSpecific<GBuffer>&;
        auto getGBuffer() const -> const FrameSpecific<GBuffer>&;

        auto getGBufferRenderPass() const -> const GBufferPass&;
        auto getCompatibleShadowRenderPass() const -> vk::RenderPass;

        auto getGlobalDataDescriptorProvider() const -> const DescriptorProviderInterface&;
        auto getSceneDescriptorProvider() const -> const DescriptorProviderInterface&;
        auto getGBufferDescriptorProvider() const -> const DescriptorProviderInterface&;
        auto getShadowDescriptorProvider() const -> const DescriptorProviderInterface&;
        auto getAssetDescriptorProvider() const -> const DescriptorProviderInterface&;

        auto getShadowPool() -> ShadowPool&;
        auto getShadowPool() const -> const ShadowPool&;

        auto getMouseDepth() const -> float;
        auto getMousePosAtDepth(const Camera& camera, float depth) const -> vec3;
        auto getMouseWorldPos(const Camera& camera) const -> vec3;

    private:
        void createGBuffer(uvec2 newSize);

        const Window& window;
        const RenderTarget* renderTarget;
        ivec2 viewportOffset{ 0, 0 };
        uvec2 viewportSize{ 1, 1 };

        const bool enableRayTracing;

        // Default render passes
        u_ptr<FrameSpecific<GBuffer>> gBuffer;
        u_ptr<GBufferPass> gBufferPass;
        u_ptr<GBufferDepthReader> mouseDepthReader;
        RenderPassShadow shadowPass;
        u_ptr<FinalLightingPass> finalLightingPass;

        // Ray tracing stuff
        u_ptr<rt::TLAS> tlas;
        u_ptr<TopLevelAccelerationStructureBuildPass> tlasBuildPass;
        u_ptr<RayTracingPass> rayTracingPass;

        // Descriptors
        GBufferDescriptor gBufferDescriptor;
        GlobalRenderDataDescriptor globalDataDescriptor;
        SceneDescriptor sceneDescriptor;
        s_ptr<AssetDescriptor> assetDescriptor;

        // Data & Assets
        ShadowPool* shadowPool;
    };
} // namespace trc
