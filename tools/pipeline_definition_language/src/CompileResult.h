#pragma once

#include <cstdint>

#include <filesystem>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>
namespace fs = std::filesystem;

#include "FlagTable.h"
#include "ShaderOutput.h"
#include "UniqueName.h"

/**
 * Either a reference or an inline object
 */
template<typename T>
using ObjectReference = std::variant<UniqueName, T>;

struct ShaderDesc
{
    std::string source;
    std::string target;
    std::unordered_map<std::string, std::string> variables;

    std::optional<ShaderOutputType> outputType;
};

struct ProgramDesc
{
    std::optional<ObjectReference<ShaderDesc>> vert;
    std::optional<ObjectReference<ShaderDesc>> geom;
    std::optional<ObjectReference<ShaderDesc>> tese;
    std::optional<ObjectReference<ShaderDesc>> tesc;
    std::optional<ObjectReference<ShaderDesc>> frag;
};

struct LayoutDesc
{
    struct PushConstantRange
    {
        size_t offset;
        size_t size;

        /** Name of a dynamically initialized default value */
        std::optional<std::string> defaultValueName;
    };

    struct Descriptor
    {
        Descriptor(std::string name, bool isStatic) : name(std::move(name)), isStatic(isStatic) {}

        std::string name;
        bool isStatic{ true };
    };

    std::vector<Descriptor> descriptors;
    std::unordered_map<std::string, std::vector<PushConstantRange>> pushConstantsPerStage;
};

struct PipelineDesc
{
    struct VertexAttribute
    {
        enum class InputRate{ ePerVertex, ePerInstance };

        size_t binding{ 0 };
        size_t stride{ 0 };
        InputRate inputRate{ InputRate::ePerVertex };
        std::vector<std::string> locationFormats{};
    };

    struct InputAssembly
    {
        std::string primitiveTopology{ "triangleList" };
        bool primitiveRestart{ false };
    };

    struct Tessellation
    {
        size_t patchControlPoints{ 0 };
    };

    struct Rasterization
    {
        enum class PolygonMode{ ePoint, eLine, eFill };
        enum class CullMode{ eNone, eFront, eBack, eBoth };
        enum class FaceWinding{ eClockwise, eCounterClockwise };

        bool depthClampEnable{ false };
        bool rasterizerDiscardEnable{ false };
        PolygonMode polygonMode{ PolygonMode::eFill };
        CullMode cullMode{ CullMode::eBack };
        FaceWinding faceWinding{ FaceWinding::eCounterClockwise };

        bool depthBiasEnable{ false };
        float depthBiasConstantFactor{ 0.0f };
        float depthBiasSlopeFactor{ 0.0f };
        float depthBiasClamp{ 0.0f };

        float lineWidth{ 1.0f };
    };

    struct Multisampling
    {
        size_t samples{ 1 };
    };

    struct DepthStencil
    {
        bool depthTestEnable{ true };
        bool depthWriteEnable{ true };

        bool depthBoundsTestEnable{ false };
        float minDepthBounds{ 0.0f };
        float maxDepthBounds{ 1.0f };

        bool stencilTestEnable{ false };
    };

    struct BlendAttachment
    {
        enum class BlendFactor
        {
            eOne, eZero,

            eConstantAlpha, eConstantColor,
            eDstAlpha, eDstColor,
            eSrcAlpha, eSrcColor,

            eOneMinusConstantAlpha, eOneMinusConstantColor,
            eOneMinusDstAlpha, eOneMinusDstColor,
            eOneMinusSrcAlpha, eOneMinusSrcColor,
        };

        enum class BlendOp{ eAdd, };

        enum Color : uint32_t
        {
            eR = 1 << 0,
            eG = 1 << 1,
            eB = 1 << 2,
            eA = 1 << 3,
        };

        bool blendEnable{ false };
        BlendFactor srcColorFactor{ BlendFactor::eOne };
        BlendFactor dstColorFactor{ BlendFactor::eZero };
        BlendOp colorBlendOp{ BlendOp::eAdd };
        BlendFactor srcAlphaFactor{ BlendFactor::eOne };
        BlendFactor dstAlphaFactor{ BlendFactor::eZero };
        BlendOp alphaBlendOp{ BlendOp::eAdd };

        uint32_t colorComponentFlags{ eR | eG | eB | eA };
    };

    ObjectReference<LayoutDesc> layout;
    std::optional<std::string> renderPassName;

    std::optional<ObjectReference<ProgramDesc>> program;
    std::vector<VertexAttribute> vertexInput{};
    InputAssembly inputAssembly{};
    Tessellation tessellation{};
    Rasterization rasterization{};
    Multisampling multisampling{};
    DepthStencil depthStencil{};
    std::vector<BlendAttachment> blendAttachments{};
    std::vector<std::string> dynamicStates{};
};

struct ComputePipelineDesc
{
    ObjectReference<LayoutDesc> layout;
    ObjectReference<ShaderDesc> shader;
};

template<typename T>
struct VariantGroup
{
    explicit VariantGroup(std::string baseName) : baseName(std::move(baseName)) {}

    std::string baseName;
    std::vector<size_t> flagTypes;
    std::unordered_map<UniqueName, T> variants;
};

struct CompileResult
{
    struct Meta
    {
        std::optional<std::string> enclosingNamespace;
    };

    template<typename T>
    using SingleOrVariant = std::variant<T, VariantGroup<T>>;

    Meta meta;

    FlagTable flagTable;

    std::unordered_map<std::string, SingleOrVariant<ShaderDesc>> shaders;
    std::unordered_map<std::string, SingleOrVariant<ProgramDesc>> programs;
    std::unordered_map<std::string, SingleOrVariant<LayoutDesc>> layouts;
    std::unordered_map<std::string, SingleOrVariant<PipelineDesc>> pipelines;
    std::unordered_map<std::string, SingleOrVariant<ComputePipelineDesc>> computePipelines;

    /**
     * Contains all shader modules defined in the compiled source. These are
     * individual shader files, *not* shader objects in the language as the
     * `shaders` member.
     *
     * Is used to collect and compile all shader sources, even those defined
     * by an inline object reference, during shader database generation.
     */
    std::vector<ShaderDesc> allShaderModules;
};
