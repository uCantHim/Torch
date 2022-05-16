#include "PipelineDataWriter.h"

#include <sstream>

#include "StringUtil.h"
#include "CompileResult.h"
#include "TorchCppWriter.h"



auto util::convertFormat(const std::string& str) -> std::string
{
    std::string result{ "e" };
    const std::string number = str.substr(str.size() - 3, 2);
    for (char c : str.substr(0, str.size() - 3))
    {
        result += capitalize(c) + number;
    }

    switch (str.back())
    {
    case 'f': result += "Sfloat"; break;
    case 'u': result += "Uint"; break;
    case 'i': result += "Sint"; break;
    }

    return result;
}

auto util::getFormatByteSize(const std::string& str) -> size_t
{
    constexpr size_t bit = 8;

    const size_t number = std::stoul(str.substr(str.size() - 3, 2));
    return (number / bit) * (str.size() - 3);
}

auto convertPrimitiveTopology(std::string str) -> std::string
{
    str[0] = capitalize(str[0]);
    str.insert(str.begin(), 'e');
    return str;
}

auto convert(PipelineDesc::Rasterization::PolygonMode mode)
{
    using T = PipelineDesc::Rasterization::PolygonMode;
    switch (mode)
    {
    case T::ePoint: return "vk::PolygonMode::ePoint";
    case T::eLine:  return "vk::PolygonMode::eLine";
    case T::eFill:  return "vk::PolygonMode::ePoint";
    }
    throw std::logic_error("");
}

auto convert(PipelineDesc::Rasterization::CullMode mode)
{
    using T = PipelineDesc::Rasterization::CullMode;
    switch (mode)
    {
    case T::eNone:  return "vk::CullModeFlagBits::eNone";
    case T::eFront: return "vk::CullModeFlagBits::eFront";
    case T::eBack:  return "vk::CullModeFlagBits::eBack";
    case T::eBoth:  return "vk::CullModeFlagBits::eFrontAndBack";
    }
    throw std::logic_error("");
}

auto convert(PipelineDesc::Rasterization::FaceWinding mode)
{
    using T = PipelineDesc::Rasterization::FaceWinding;
    switch (mode)
    {
    case T::eClockwise:        return "vk::FrontFace::eClockwise";
    case T::eCounterClockwise: return "vk::FrontFace::eCounterClockwise";
    }
    throw std::logic_error("");
}

auto convert(PipelineDesc::BlendAttachment::BlendFactor mode)
{
    using T = PipelineDesc::BlendAttachment::BlendFactor;
    switch (mode)
    {
    case T::eOne:                   return "vk::BlendFactor::eOne";
    case T::eZero:                  return "vk::BlendFactor::eZero";
    case T::eConstantAlpha:         return "vk::BlendFactor::eConstantAlpha";
    case T::eConstantColor:         return "vk::BlendFactor::eConstantColor";
    case T::eDstAlpha:              return "vk::BlendFactor::eDstAlpha";
    case T::eDstColor:              return "vk::BlendFactor::eDstColor";
    case T::eSrcAlpha:              return "vk::BlendFactor::eSrcAlpha";
    case T::eSrcColor:              return "vk::BlendFactor::eSrcColor";
    case T::eOneMinusConstantAlpha: return "vk::BlendFactor::eOneMinusConstantAlpha";
    case T::eOneMinusConstantColor: return "vk::BlendFactor::eOneMinusConstantColor";
    case T::eOneMinusDstAlpha:      return "vk::BlendFactor::eOneMinusDstAlpha";
    case T::eOneMinusDstColor:      return "vk::BlendFactor::eOneMinusDstColor";
    case T::eOneMinusSrcAlpha:      return "vk::BlendFactor::eOneMinusSrcAlpha";
    case T::eOneMinusSrcColor:      return "vk::BlendFactor::eOneMinusSrcColor";
    }
    throw std::logic_error("");
}

auto convert(PipelineDesc::BlendAttachment::BlendOp mode)
{
    using T = PipelineDesc::BlendAttachment::BlendOp;
    switch (mode)
    {
    case T::eAdd: return "vk::BlendOp::eAdd";
    }
    throw std::logic_error("");
}

auto convertColorComponentFlags(uint flags) -> std::string
{
    using Color = PipelineDesc::BlendAttachment::Color;

    std::string result;
    if (flags & Color::eR) result += "vk::ColorComponentFlagBits::eR | ";
    if (flags & Color::eG) result += "vk::ColorComponentFlagBits::eG | ";
    if (flags & Color::eB) result += "vk::ColorComponentFlagBits::eB | ";
    if (flags & Color::eA) result += "vk::ColorComponentFlagBits::eA | ";
    if (!result.empty()) result.erase(result.end() - 3, result.end());

    return result;
}



auto makePipelineDefinitionDataInit(const PipelineDesc& pipeline, LineWriter& nl) -> std::string
{
    std::stringstream ss;
    ss << "trc::PipelineDefinitionData{";
    ++nl;

    // Write input bindings
    ss << nl++ << ".inputBindings{";
    for (const auto& attrib : pipeline.vertexInput)
    {
        using Rate = PipelineDesc::VertexAttribute::InputRate;
        ss << nl << "vk::VertexInputBindingDescription("
           << attrib.binding << ", " << attrib.stride << ", "
           << "vk::VertexInputRate::"
               << (attrib.inputRate == Rate::ePerVertex ? "eVertex" : "eInstance")
           << "),";
    }
    ss << --nl << "},";  // .inputBindings{
    // Write input attributes
    ss << nl++ << ".attributes{";
    for (const auto& attrib : pipeline.vertexInput)
    {
        for (size_t loc = 0, offset = 0; const auto& format : attrib.locationFormats)
        {
            ss << nl << "vk::VertexInputAttributeDescription("
               << loc++ << ", " << attrib.binding << ", "
               << "vk::Format::" << util::convertFormat(format) << ", " << offset
               << "),";
            offset += util::getFormatByteSize(format);
        }
    }
    ss << --nl << "},";  // .inputAttributes{
    ss << nl << ".vertexInput{},";  // Is set automatically from the two previous attributes

    // Write input assembly
    ss << nl << ".inputAssembly{ {}, " << "vk::PrimitiveTopology::"
       << convertPrimitiveTopology(pipeline.inputAssembly.primitiveTopology) << ", "
       << std::boolalpha << pipeline.inputAssembly.primitiveRestart << " },";

    // Write tessellation
    ss << nl << ".tessellation{ {}, " << pipeline.tessellation.patchControlPoints << " },";

    // Write empty viewport properties for completeness
    ss << nl << ".viewports{}," << nl << ".scissorRects{}," << nl << ".viewport{},";

    // Write rasterization
    const auto& r = pipeline.rasterization;
    ss << nl << ".rasterization{"
       << ++nl << "{}," << std::boolalpha
       << nl << r.depthClampEnable << ", // depth clamp enable"
       << nl << r.rasterizerDiscardEnable << ", // rasterizer discard enable"
       << nl << convert(r.polygonMode) << ", "
             << convert(r.cullMode) << ", "
             << convert(r.faceWinding) << ","
       << nl << r.depthBiasEnable << ", " << r.depthBiasConstantFactor << ", "
             << r.depthBiasClamp << ", " << r.depthBiasSlopeFactor << ", "
             << "// depth bias settings"
       << nl << r.lineWidth << " // line width"
       << --nl << "},";

    // Write multisampling
    const auto& m = pipeline.multisampling;
    ss << nl << ".multisampling{" << std::boolalpha
       << ++nl << "{},"
       << nl << "vk::SampleCountFlagBits::e" << std::to_string(m.samples) << ","
       << nl << false << ", 0.0f, nullptr, " << false << ", " << false
       << --nl << "},";

    // Write depth/stencil
    const auto& ds = pipeline.depthStencil;
    ss << nl << ".depthStencil{" << std::boolalpha
       << ++nl << "{},"
       << nl << ds.depthTestEnable << ", // depth test enable"
       << nl << ds.depthWriteEnable << ", // depth write enable"
       << nl << "vk::CompareOp::eLess, " << ds.depthBoundsTestEnable << ","
       << nl << ds.stencilTestEnable << ", // stencil test enable"
       << nl << "vk::StencilOpState(), vk::StencilOpState(),"
       << nl << ds.minDepthBounds << ", " << ds.maxDepthBounds << " // min/max depth bounds"
       << --nl << "},";

    // Write color blend attachments
    ss << nl++ << ".colorBlendAttachments{";
    for (const auto& att : pipeline.blendAttachments)
    {
        if (!att.blendEnable)
        {
            ss << nl << "vk::PipelineColorBlendAttachmentState{ false },";
            continue;
        }
        ss << nl++ << "vk::PipelineColorBlendAttachmentState{"
           << nl << att.blendEnable << ","
           << nl << convert(att.srcColorFactor) << ", " << convert(att.dstColorFactor) << ", "
                 << convert(att.colorBlendOp) << ",  // color blending"
           << nl << convert(att.srcAlphaFactor) << ", " << convert(att.dstAlphaFactor) << ", "
                 << convert(att.alphaBlendOp) << ",  // alpha blending"
           << nl << convertColorComponentFlags(att.colorComponentFlags)
           << --nl << "},";
    }
    ss << --nl << "},";
    ss << ".colorBlending{},";

    // Write dynamic state
    ss << nl++ << ".dynamicStates{";
    for (const auto& state : pipeline.dynamicStates) {
        ss << nl << "vk::DynamicState::" << state << ",";
    }
    ss << --nl << "},";
    ss << ".dynamicState{},";

    // End
    ss << --nl << "}";  // PipelineDefinitionData{

    return ss.str();
}
