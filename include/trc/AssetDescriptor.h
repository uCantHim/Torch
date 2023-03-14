#pragma once

#include <unordered_map>

#include "trc/Types.h"
#include "trc/assets/AssetBase.h"
#include "trc/assets/SharedDescriptorSet.h"
#include "trc/base/Device.h"
#include "trc/core/DescriptorProvider.h"

namespace trc
{
    struct AssetDescriptorCreateInfo
    {
        // Ray-tracing specific. The maximum number of geometries for which the
        // descriptor can hold vertex and index data.
        ui32 maxGeometries;

        // The maximum number of texture samplers that may exist in the
        // descriptor.
        ui32 maxTextures;
    };

    enum class AssetDescriptorBinding
    {
        // Ray-tracing specific. An array of index buffers. Contains an index
        // buffer for each registered geometry.
        eGeometryIndexBuffers,

        // Ray-tracing specific. An array of vertex buffers. Contains a vertex
        // buffer for each registered geometry.
        eGeometryVertexBuffers,

        // An array of samplers. Contains one sampler for each registered
        // texture.
        eTextureSamplers,

        // A buffer that holds information about the data layout in the
        // `AssetDescriptorBinding::eAnimationData` binding.
        eAnimationMetadata,

        // A large buffer that holds bone transformation matrices for all
        // animations.
        eAnimationData,
    };

    /**
     * @brief A descriptor set for all native Torch assets
     */
    class AssetDescriptor : public DescriptorProviderInterface
    {
    public:
        using Binding = SharedDescriptorSet::Binding;

        AssetDescriptor(const Device& device, const AssetDescriptorCreateInfo& info);

        void update(const Device& device);

        /**
         * @return Binding A handle to the specified descriptor binding.
         */
        auto getBinding(AssetDescriptorBinding binding) -> Binding;

        /**
         * @return Binding A handle to the specified descriptor binding.
         */
        auto getBinding(AssetDescriptorBinding binding) const -> const Binding;

        /**
         * @return ui32 The specified binding's index in the descriptor set.
         */
        auto getBindingIndex(AssetDescriptorBinding binding) const -> ui32;

        auto getDescriptorSetLayout() const noexcept -> vk::DescriptorSetLayout override;
        void bindDescriptorSet(
            vk::CommandBuffer cmdBuf,
            vk::PipelineBindPoint bindPoint,
            vk::PipelineLayout pipelineLayout,
            ui32 setIndex
        ) const override;

    private:
        s_ptr<SharedDescriptorSet> descSet;
        std::unordered_map<AssetDescriptorBinding, Binding> bindings;
    };
} // namespace trc
