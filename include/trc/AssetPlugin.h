#pragma once

#include "trc/AssetDescriptor.h"
#include "trc/core/RenderPlugin.h"

namespace trc
{
    class AssetRegistry;
    class AssetPlugin;

    auto buildAssetPlugin(AssetRegistry& reg,
                          const AssetDescriptorCreateInfo& createInfo)
        -> PluginBuilder;

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
        AssetPlugin(const Instance& instance,
                    AssetRegistry& registry,
                    const AssetDescriptorCreateInfo& createInfo);

        void defineRenderStages(RenderGraph& renderGraph) override;
        void defineResources(ResourceConfig& config) override;

        auto createGlobalResources(RenderPipelineContext& ctx) -> u_ptr<GlobalResources> override;

    private:
        class UpdateConfig : public GlobalResources
        {
        public:
            explicit UpdateConfig(AssetPlugin& parent);

            void registerResources(ResourceStorage& resources) override;
            void hostUpdate(RenderPipelineContext& ctx) override;
            void createTasks(GlobalUpdateTaskQueue& taskQueue) override;

        private:
            AssetPlugin* parent;
        };

        s_ptr<AssetDescriptor> assetDescriptor;
        AssetRegistry* registry;
    };
} // namespace trc
