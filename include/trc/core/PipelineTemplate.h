#pragma once

#include <string>
#include <vector>
#include <optional>

#include "trc/Types.h"
#include "trc/core/DefaultPipelineValues.h"
#include "trc/core/PipelineLayout.h"
#include "trc/core/RenderConfiguration.h"

namespace trc {
    class ShaderProgram;
}

namespace trc
{
    class SpecializationConstantStorage
    {
    public:
        SpecializationConstantStorage() = default;

        /**
         * @param ui32 constantId ID of the specialization constant
         * @param T&   value      Data for the specialization constant
         */
        template<typename T>
        void set(ui32 constantId, T&& value);

        /**
         * @brief Set raw data for a specialization constant
         *
         * @param ui32   constantId ID of the specialization constant
         * @param void*  data       Data for the specialization constant
         * @param size_t size       Size of `data`
         */
        void set(ui32 constantId, const void* data, size_t size);

        bool empty() const;

        /**
         * Adding constants with SpecializationConstantStorage::set<> after
         * a call to this function is dangerous because it might invalidate
         * pointers to the internal data storage, which are used by the
         * resulting vk::SpecializationInfo.
         */
        auto makeSpecializationInfo() const -> vk::SpecializationInfo;

    private:
        std::vector<vk::SpecializationMapEntry> entries{};
        std::vector<ui8> data{};
    };

    struct ProgramDefinitionData
    {
        struct ShaderStage
        {
            std::vector<ui32> code;
            SpecializationConstantStorage specConstants{};
        };

        auto makeProgram(const Device& device) const -> ShaderProgram;

        std::unordered_map<vk::ShaderStageFlagBits, ShaderStage> stages;
    };

    struct PipelineDefinitionData
    {
        std::vector<vk::VertexInputBindingDescription> inputBindings;
        std::vector<vk::VertexInputAttributeDescription> attributes;

        vk::PipelineInputAssemblyStateCreateInfo inputAssembly{ DEFAULT_INPUT_ASSEMBLY };

        vk::PipelineTessellationStateCreateInfo tessellation{ DEFAULT_TESSELLATION };

        std::vector<vk::Viewport> viewports;
        std::vector<vk::Rect2D> scissorRects;

        vk::PipelineRasterizationStateCreateInfo rasterization{ DEFAULT_RASTERIZATION };
        vk::PipelineMultisampleStateCreateInfo multisampling{ DEFAULT_MULTISAMPLING };
        vk::PipelineDepthStencilStateCreateInfo depthStencil{ DEFAULT_DEPTH_STENCIL };

        std::vector<vk::PipelineColorBlendAttachmentState> colorBlendAttachments;
        vk::PipelineColorBlendStateCreateInfo colorBlending;

        std::vector<vk::DynamicState> dynamicStates;
    };

    /**
     * The PipelineTemplate automatically sets viewport and scissor states
     * as dynamic states if the pipeline definition contains no viewports
     * or scissor rectangles, respectively.
     */
    class PipelineTemplate
    {
    public:
        PipelineTemplate() = default;

        /**
         * @brief Create a pipeline template
         */
        PipelineTemplate(ProgramDefinitionData program, PipelineDefinitionData pipeline);

        auto getProgramData() const -> const ProgramDefinitionData&;
        auto getPipelineData() const -> const PipelineDefinitionData&;

    private:
        ProgramDefinitionData program;
        PipelineDefinitionData data;
    };

    class ComputePipelineTemplate
    {
    public:
        ComputePipelineTemplate() = default;
        explicit ComputePipelineTemplate(std::vector<ui32> shaderCode);

        void setProgramCode(std::vector<ui32> code);

        template<typename T>
        inline void setSpecializationConstant(ui32 constantId, T&& data)
        {
            specConstants.set(constantId, std::forward<T>(data));
        }

        auto getShaderCode() const -> const std::vector<ui32>&;
        auto getSpecializationConstants() const -> const SpecializationConstantStorage&;
        auto getEntryPoint() const -> const std::string&;

    private:
        std::vector<ui32> shaderCode;
        SpecializationConstantStorage specConstants;
        std::string entryPoint{ "main" };
    };

    /**
     * @brief Create a graphics pipeline from a template
     *
     * Adds viewport and scissor to the dynamic states automatically if
     * no `vk::Viewport` or `vk::Rect2D` structs, respectively, are
     * specified in the pipeline template.
     */
    auto makeGraphicsPipeline(const Device& device,
                              const PipelineTemplate& _template,
                              PipelineLayout& layout,
                              vk::RenderPass renderPass,
                              ui32 subPass
        ) -> Pipeline;

    /**
     * @brief Create a graphics pipeline from a template
     *
     * Create the pipeline for dynamic rendering, without a render pass.
     *
     * Adds viewport and scissor to the dynamic states automatically if
     * no `vk::Viewport` or `vk::Rect2D` structs, respectively, are
     * specified in the pipeline template.
     */
    auto makeGraphicsPipeline(const Device& device,
                              const PipelineTemplate& _template,
                              PipelineLayout& layout,
                              const vk::PipelineRenderingCreateInfo& renderingInfo
        ) -> Pipeline;

    /**
     * @brief Create a compute pipeline from a template
     */
    auto makeComputePipeline(const Device& device,
                             const ComputePipelineTemplate& _template,
                             PipelineLayout& layout
        ) -> Pipeline;



    template<typename T>
    void SpecializationConstantStorage::set(ui32 constantId, T&& value)
    {
        // Copy new entry's data
        const ui32 offset = data.size();
        const ui32 minSize = offset + sizeof(T);
        assert(data.size() < minSize);

        data.resize(minSize);
        memcpy(data.data() + offset, &value, sizeof(T));

        // Add map entry
        entries.emplace_back(vk::SpecializationMapEntry(constantId, offset, sizeof(T)));
    }
} // namespace trc
