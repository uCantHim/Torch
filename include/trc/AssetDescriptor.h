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

        // The maximum number of glyph maps that may exist in the descriptor.
        ui32 maxFonts;
    };

    enum class AssetDescriptorBinding
    {
        // Ray-tracing specific. An array of index buffers. Contains an index
        // buffer for each registered geometry.
        //
        // GLSL format: `std430 buffer { uint indices[]; }`
        eGeometryIndexBuffers,

        // Ray-tracing specific. An array of vertex buffers. Contains a vertex
        // buffer for each registered geometry.
        //
        // GLSL format: `std430 buffer { Vertex vertices[]; }`
        eGeometryVertexBuffers,

        // An array of samplers. Contains one sampler for each registered
        // texture.
        //
        // GLSL format: `sampler2D[]`
        eTextureSamplers,

        // An array of samplers. Contains one sampler for each registered
        // glyph map.
        //
        // GLSL format: `sampler2D[]`
        eGlyphMapSamplers,

        // A buffer that holds information about the data layout in the
        // `AssetDescriptorBinding::eAnimationData` binding.
        //
        // GLSL format: `std430 buffer { AnimationMetaData meta[]; }`
        eAnimationMetadata,

        // A large buffer that holds bone transformation matrices for all
        // animations.
        //
        // GLSL format: `std140 buffer { mat4 boneMatrices[]; }`
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
        static constexpr auto getBindingIndex(AssetDescriptorBinding binding) -> ui32 {
            return static_cast<ui32>(binding);
        }

        auto getDescriptorSetLayout() const noexcept -> vk::DescriptorSetLayout;
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
