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
         * @param ui32 offset Offset of `data`
         */
        template<typename T>
        void set(ui32 constantId, T&& value);

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
            std::string code;
            SpecializationConstantStorage specConstants{};
        };

        auto makeProgram(const Device& device) const -> ShaderProgram;

        std::unordered_map<vk::ShaderStageFlagBits, ShaderStage> stages;
    };

    struct PipelineDefinitionData
    {
        std::vector<vk::VertexInputBindingDescription> inputBindings;
        std::vector<vk::VertexInputAttributeDescription> attributes;
        vk::PipelineVertexInputStateCreateInfo vertexInput;

        vk::PipelineInputAssemblyStateCreateInfo inputAssembly{ DEFAULT_INPUT_ASSEMBLY };

        vk::PipelineTessellationStateCreateInfo tessellation{ DEFAULT_TESSELLATION };

        std::vector<vk::Viewport> viewports;
        std::vector<vk::Rect2D> scissorRects;
        vk::PipelineViewportStateCreateInfo viewport;

        vk::PipelineRasterizationStateCreateInfo rasterization{ DEFAULT_RASTERIZATION };
        vk::PipelineMultisampleStateCreateInfo multisampling{ DEFAULT_MULTISAMPLING };
        vk::PipelineDepthStencilStateCreateInfo depthStencil{ DEFAULT_DEPTH_STENCIL };

        std::vector<vk::PipelineColorBlendAttachmentState> colorBlendAttachments;
        vk::PipelineColorBlendStateCreateInfo colorBlending;

        std::vector<vk::DynamicState> dynamicStates;
        vk::PipelineDynamicStateCreateInfo dynamicState;
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
        PipelineTemplate(ProgramDefinitionData program, PipelineDefinitionData pipeline);

        auto getProgramData() const -> const ProgramDefinitionData&;
        auto getPipelineData() const -> const PipelineDefinitionData&;

    private:
        void compileData();

        ProgramDefinitionData program;
        PipelineDefinitionData data;
    };

    class ComputePipelineTemplate
    {
    public:
        ComputePipelineTemplate() = default;
        explicit ComputePipelineTemplate(std::string shaderCode);

        void setProgramCode(std::string code);

        template<typename T>
        inline void setSpecializationConstant(ui32 constantId, T&& data)
        {
            specConstants.set(constantId, std::forward<T>(data));
        }

        auto getShaderCode() const -> const std::string&;
        auto getSpecializationConstants() const -> const SpecializationConstantStorage&;
        auto getEntryPoint() const -> const std::string&;

    private:
        std::string shaderCode;
        SpecializationConstantStorage specConstants;
        std::string entryPoint{ "main" };
    };

    /**
     * @brief Create a graphics pipeline from a template
     */
    auto makeGraphicsPipeline(const Device& device,
                              const PipelineTemplate& _template,
                              PipelineLayout& layout,
                              vk::RenderPass renderPass,
                              ui32 subPass
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
        memcpy(data.data(), &value, sizeof(T));

        // Add map entry
        entries.emplace_back(vk::SpecializationMapEntry(constantId, offset, sizeof(T)));
    }
} // namespace trc
