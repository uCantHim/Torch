#pragma once

#include <string>
#include <vector>
#include <optional>

#include "../Types.h"
#include "DefaultPipelineValues.h"
#include "PipelineLayout.h"
#include "RenderConfiguration.h"

namespace vkb {
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

        auto makeProgram(const vkb::Device& device) const -> vkb::ShaderProgram;

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

    class GraphicsPipelineBuilder;

    /**
     * @brief
     */
    class PipelineTemplate
    {
    public:
        PipelineTemplate() = default;
        PipelineTemplate(ProgramDefinitionData program, PipelineDefinitionData pipeline);
        PipelineTemplate(ProgramDefinitionData program,
                         PipelineDefinitionData pipeline,
                         PipelineLayout::ID layout,
                         const RenderPassName& renderPass);

        /**
         * @brief Alter the template with a pipeline builder
         *
         * Creates a copy of the template and wraps it into a pipeline
         * builder, from where it can be conveniently modified. The
         * original PipelineTemplate object on which this method is called
         * will not be altered.
         */
        auto modify() const -> GraphicsPipelineBuilder;

        void setLayout(PipelineLayout::ID layout);
        void setRenderPass(const RenderPassName& name);

        auto getLayout() const -> PipelineLayout::ID;
        auto getRenderPass() const -> const RenderPassName&;
        auto getProgramData() const -> const ProgramDefinitionData&;
        auto getPipelineData() const -> const PipelineDefinitionData&;

    private:
        void compileData();

        PipelineLayout::ID layout{ PipelineLayout::ID::NONE };
        RenderPassName renderPassName{};

        ProgramDefinitionData program;
        PipelineDefinitionData data;
    };

    class ComputePipelineTemplate
    {
    public:
        ComputePipelineTemplate() = default;

        void setProgramCode(std::string code);
        void setLayout(PipelineLayout::ID layoutId);

        template<typename T>
        inline void setSpecializationConstant(ui32 constantId, T&& data)
        {
            specConstants.set(constantId, std::forward<T>(data));
        }

        auto getLayout() const -> PipelineLayout::ID;
        auto getShaderCode() const -> const std::string&;
        auto getSpecializationConstants() const -> const SpecializationConstantStorage&;
        auto getEntryPoint() const -> const std::string&;

    private:
        PipelineLayout::ID layout{ PipelineLayout::ID::NONE };

        std::string shaderCode;
        SpecializationConstantStorage specConstants;
        std::string entryPoint{ "main" };
    };

    /**
     * @brief Create a graphics pipeline from a template
     */
    auto makeGraphicsPipeline(const PipelineTemplate& _template,
                              const vkb::Device& device,
                              PipelineLayout& layout,
                              vk::RenderPass renderPass,
                              ui32 subPass
                              ) -> Pipeline;

    /**
     * @brief Create a compute pipeline from a template
     */
    auto makeComputePipeline(const ComputePipelineTemplate& _template,
                             vk::Device device,
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
