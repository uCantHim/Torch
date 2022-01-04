#pragma once

#include <array>
#include <vector>

#include <vkb/ShaderProgram.h>

#include "../Types.h"
#include "PipelineTemplate.h"
#include "PipelineRegistry.h"

namespace trc
{
    class GraphicsPipelineBuilder
    {
    public:
        using Self = GraphicsPipelineBuilder;
        using ShaderCode = std::string;

        GraphicsPipelineBuilder() = default;
        explicit GraphicsPipelineBuilder(const PipelineTemplate& _template);


        ////////////////////////
        //  Program building  //
        ////////////////////////

        auto setVertexShader(ShaderCode code) -> Self&;
        auto setGeometryShader(ShaderCode code) -> Self&;
        auto setTesselationShader(ShaderCode control, ShaderCode eval) -> Self&;

        auto setTaskShader(ShaderCode code) -> Self&;
        auto setMeshShader(ShaderCode code) -> Self&;

        auto setFragmentShader(ShaderCode code) -> Self&;

        auto setShader(vk::ShaderStageFlagBits stage, ShaderCode code) -> Self&;

        auto setProgram(ShaderCode vertex, ShaderCode fragment) -> Self&;
        auto setProgram(ShaderCode vertex, ShaderCode geom, ShaderCode fragment) -> Self&;
        auto setProgram(ShaderCode vertex, ShaderCode tesc, ShaderCode tese, ShaderCode fragment)
            -> Self&;
        auto setProgram(ShaderCode vertex,
                        ShaderCode tesc,
                        ShaderCode tese,
                        ShaderCode geometry,
                        ShaderCode fragment)
            -> Self&;

        auto setMeshShadingProgram(std::optional<ShaderCode> task,
                                   ShaderCode mesh,
                                   ShaderCode fragment)
            -> Self&;

        template<typename T>
        inline auto setVertexSpecializationConstant(ui32 constantId, T&& data) -> Self&;
        template<typename T>
        inline auto setFragmentSpecializationConstant(ui32 constantId, T&& data) -> Self&;
        template<typename T>
        inline auto setGeometrySpecializationConstant(ui32 constantId, T&& data) -> Self&;
        template<typename T>
        inline auto setTessControlSpecializationConstant(ui32 constantId, T&& data) -> Self&;
        template<typename T>
        inline auto setTessEvalSpecializationConstant(ui32 constantId, T&& data) -> Self&;
        template<typename T>
        inline auto setTaskSpecializationConstant(ui32 constantId, T&& data) -> Self&;
        template<typename T>
        inline auto setMeshSpecializationConstant(ui32 constantId, T&& data) -> Self&;

        template<typename T>
        inline auto setSpecializationConstant(vk::ShaderStageFlagBits stage,
                                              ui32 constantId,
                                              T&& data)
            -> Self&;


        /////////////////////////
        //  Pipeline settings  //
        /////////////////////////

        auto addVertexInputBinding(vk::VertexInputBindingDescription inputBinding,
                                   std::vector<vk::VertexInputAttributeDescription> attributes) -> Self&;
        auto setVertexInput(const std::vector<vk::VertexInputBindingDescription>& inputBindings,
                            const std::vector<vk::VertexInputAttributeDescription>& attributes) -> Self&;

        auto setInputAssembly(vk::PipelineInputAssemblyStateCreateInfo inputAssembly) -> Self&;
        auto setPrimitiveTopology(vk::PrimitiveTopology topo) -> Self&;

        auto setTessellation(vk::PipelineTessellationStateCreateInfo tessellation) -> Self&;

        auto addViewport(vk::Viewport viewport) -> Self&;
        auto addScissorRect(vk::Rect2D scissorRect) -> Self&;

        auto setRasterization(vk::PipelineRasterizationStateCreateInfo rasterization) -> Self&;
        auto setPolygonMode(vk::PolygonMode polyMode) -> Self&;
        auto setCullMode(vk::CullModeFlags cullMode) -> Self&;
        auto setFrontFace(vk::FrontFace frontFace) -> Self&;

        auto setMultisampling(vk::PipelineMultisampleStateCreateInfo multisampling) -> Self&;
        auto setSampleCount(vk::SampleCountFlagBits sampleCount) -> Self&;

        auto setDepthStencilTests(vk::PipelineDepthStencilStateCreateInfo depthStencil) -> Self&;
        auto enableDepthTest() -> Self&;
        auto disableDepthTest() -> Self&;
        auto enableDepthWrite() -> Self&;
        auto disableDepthWrite() -> Self&;

        auto addColorBlendAttachment(vk::PipelineColorBlendAttachmentState blendAttachment) -> Self&;
        /**
         * Call this after all color blend attachments have been specified via
         * addColorBlendAttachment().
         */
        auto setColorBlending(vk::PipelineColorBlendStateCreateFlags flags,
                              vk::Bool32 logicOpEnable, vk::LogicOp logicalOperation,
                              std::array<float, 4> blendConstants) -> Self&;

        auto disableBlendAttachments(ui32 numAttachments) -> Self&;

        auto addDynamicState(vk::DynamicState dynamicState) -> Self&;

        /**
         * @build Finish the build process and create a pipeline template
         */
        auto build() const -> PipelineTemplate;

        auto build(const vkb::Device& device,
                   PipelineLayout& layout,
                   vk::RenderPass renderPass,
                   ui32 subPass)
            -> Pipeline;

        /**
         * @brief Build the pipeline and register it at a pipeline registry
         *
         * @param PipelineLayout::ID layout The pipeline's layout
         * @param RenderPassName renderPass Name of a render pass with
         *        which the pipeline will be compatible
         */
        template<RenderConfigType T>
        auto registerPipeline(PipelineLayout::ID layout, const RenderPassName& renderPass) const
            -> Pipeline::ID;

    private:
        ProgramDefinitionData program;
        PipelineDefinitionData data;
    };

    auto buildGraphicsPipeline() -> GraphicsPipelineBuilder;



    template<RenderConfigType T>
    auto GraphicsPipelineBuilder::registerPipeline(
        PipelineLayout::ID layout,
        const RenderPassName& renderPass) const -> Pipeline::ID
    {
        return PipelineRegistry<T>::registerPipeline(build(), layout, renderPass);
    }

    template<typename T>
    inline auto GraphicsPipelineBuilder::setVertexSpecializationConstant(
        ui32 constantId, T&& data) -> Self&
    {
        return setSpecializationConstant(
            vk::ShaderStageFlagBits::eVertex, constantId, std::forward<T>(data)
        );
    }

    template<typename T>
    inline auto GraphicsPipelineBuilder::setFragmentSpecializationConstant(
        ui32 constantId, T&& data) -> Self&
    {
        return setSpecializationConstant(
            vk::ShaderStageFlagBits::eFragment, constantId, std::forward<T>(data)
        );
    }

    template<typename T>
    inline auto GraphicsPipelineBuilder::setGeometrySpecializationConstant(
        ui32 constantId, T&& data) -> Self&
    {
        return setSpecializationConstant(
            vk::ShaderStageFlagBits::eGeometry, constantId, std::forward<T>(data)
        );
    }

    template<typename T>
    inline auto GraphicsPipelineBuilder::setTessControlSpecializationConstant(
        ui32 constantId, T&& data) -> Self&
    {
        return setSpecializationConstant(
            vk::ShaderStageFlagBits::eTessellationControl, constantId, std::forward<T>(data)
        );
    }

    template<typename T>
    inline auto GraphicsPipelineBuilder::setTessEvalSpecializationConstant(
        ui32 constantId, T&& data) -> Self&
    {
        return setSpecializationConstant(
            vk::ShaderStageFlagBits::eTessellationEvaluation, constantId, std::forward<T>(data)
        );
    }

    template<typename T>
    inline auto GraphicsPipelineBuilder::setTaskSpecializationConstant(
        ui32 constantId, T&& data) -> Self&
    {
        return setSpecializationConstant(
            vk::ShaderStageFlagBits::eTaskNV, constantId, std::forward<T>(data)
        );
    }

    template<typename T>
    inline auto GraphicsPipelineBuilder::setMeshSpecializationConstant(
        ui32 constantId, T&& data) -> Self&
    {
        return setSpecializationConstant(
            vk::ShaderStageFlagBits::eMeshNV, constantId, std::forward<T>(data)
        );
    }

    template<typename T>
    inline auto GraphicsPipelineBuilder::setSpecializationConstant(
        vk::ShaderStageFlagBits stage, ui32 constantId, T&& data) -> Self&
    {
        auto it = program.stages.find(stage);
        if (it != program.stages.end()) {
            it->second.specConstants.set(constantId, std::forward<T>(data));
        }
        else {
            throw Exception(
                "[In GraphicsPipelineBuilder::setSpecializationConstant]: Shader stage "
                + vk::to_string(stage) + " is not present in the pipeline builder"
            );
        }
        return *this;
    }
} // namespace trc
