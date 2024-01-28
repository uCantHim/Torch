#pragma once

#include "trc/core/RenderPlugin.h"

namespace trc
{
    class AssetDescriptor;
    class AssetRegistry;

    /**
     * Provides definitions for the asset descriptor and performs device updates
     * for an asset registry and the corresponding descriptor.
     */
    class AssetPlugin : public RenderPlugin
    {
    public:
        /**
         * Device data provided by the asset registry
         */
        static constexpr auto ASSET_DESCRIPTOR{ "asset_registry" };

        /**
         * @param AssetRegistry&         assetRegistry   The asset registry to
         *        which `assetDescriptor` points.
         * @param s_ptr<AssetDescriptor> assetDescriptor The instance that makes
         *        makes an AssetRegistry's data available to the device. Create
         *        this descriptor via `makeAssetDescriptor`. That function
         *        registers all asset modules that are necessary to use Torch's
         *        default asset types at an asset registry and builds a
         *        descriptor for their data.
         */
        AssetPlugin(AssetRegistry& registry,
                    s_ptr<AssetDescriptor> assetDescriptor);

        void registerRenderStages(RenderGraph& renderGraph) override;
        void defineResources(ResourceConfig& config) override;
        void registerSceneModules(SceneBase& scene) override;

        auto createDrawConfig(const Device& device, Viewport renderTarget)
            -> u_ptr<DrawConfig> override;

    private:
        class UpdateConfig : public DrawConfig
        {
        public:
            explicit UpdateConfig(AssetPlugin& parent);

            void registerResources(ResourceStorage& resources) override;

            void update(const Device& device, SceneBase& scene, const Camera& camera) override;
            void createTasks(SceneBase& scene, TaskQueue& taskQueue) override;

        private:
            AssetPlugin* parent;
        };

        s_ptr<AssetDescriptor> assetDescriptor;
        AssetRegistry* registry;
    };
} // namespace trc
