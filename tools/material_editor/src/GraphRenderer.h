#pragma once

#include <vector>

#include <trc/base/FrameSpecificObject.h>
#include <trc/base/Buffer.h>

#include "Font.h"
#include "GraphScene.h"
#include "MaterialGraph.h"

namespace graph
{
    constexpr vec4 kNodeColor{ 0.025f, 0.025f, 0.025f, 1.0f };
    constexpr vec4 kSocketColor{ 0.8f, 0.2f, 0.2f, 1.0f };
    constexpr vec4 kLinkColor{ 0.8f, 0.8f, 0.8f, 1.0f };

    constexpr vec4 kSeparatorColor{ vec3(kNodeColor) * 3.0f, 1.0f };
    constexpr vec4 kTextColor{ 1.0f };

    constexpr vec4 kHighlightColor{ 1.0f, 0.7f, 0.2f, 1.0f };
    constexpr vec4 kSelectColor{ 1.0f, 1.0f, 1.0f, 1.0f };

    // Color multiplier to highlight things by brightnening them
    constexpr float kHighlightColorModifier{ 1.8f };

    /**
     * Apply a multiplicative modifier to a color, keeping the color channels
     * in the bounds [0, 1].
     */
    constexpr auto applyColorModifier(auto color, float mod) -> decltype(color)
    {
        using vec = decltype(color);
        return glm::clamp(color * mod, vec(0.0f), vec(1.0f));
    }
} // namespace graph

/**
 * @brief A renderable representation of a material graph
 */
struct GraphRenderData
{
    struct DrawGroup;

    GraphRenderData() {
        beginGroup();
    }

    void beginGroup();
    auto curGroup() -> DrawGroup&;

    void pushNode(vec2 pos, vec2 size, vec4 color);
    void pushSocket(vec2 pos, vec2 size, vec4 color);
    void pushLink(vec2 from, vec2 to, vec4 color);
    void pushText(vec2 pos, float scale, std::string_view str, vec4 color, Font& font);
    void pushLetter(vec2 pos, float scale, trc::CharCode c, vec4 color, Font& font);

    void pushBorder(vec2 pos, vec2 size, vec4 color);
    void pushSeparator(vec2 pos, float size, vec4 color);

    void clear();

    enum class Geometry { eQuad, eLine, eGlyph };

    struct Primitive
    {
        Geometry geometryType;

        /** List of vec4{ posX, posY, sizeX, sizeY } */
        std::vector<vec4> dimensions{};
        /** List of vec4{ offsetX, offsetY, sizeX, sizeY } */
        std::vector<vec4> uvDims{};
        std::vector<vec4> colors{};

        void pushItem(vec4 dim, vec4 uvDim, vec4 color);
        void pushDefaultUvs();
    };

    struct DrawGroup
    {
        Primitive quads{ .geometryType=Geometry::eQuad };
        Primitive lines{ .geometryType=Geometry::eLine };
        Primitive glyphs{ .geometryType=Geometry::eGlyph };
    };

    static constexpr vec4 kDefaultUv{ 0.0f, 0.0f, 1.0f, 1.0f };

    std::vector<DrawGroup> groups;
};

/**
 * @brief Compile a material graph into a renderable representation
 */
auto buildRenderData(const GraphScene& graph, Font& font) -> GraphRenderData;

class MaterialGraphRenderer
{
public:
    MaterialGraphRenderer(const trc::Device& device,
                          const trc::FrameClock& clock,
                          const trc::Image& fontImage);

    void uploadData(const GraphRenderData& data);
    void draw(vk::CommandBuffer cmdBuf, trc::RenderConfig& conf);

    auto getTextureDescriptor() const -> const trc::DescriptorProviderInterface&;

private:
    static constexpr size_t kVertexPosOffset{ 0 };
    static constexpr size_t kVertexPosSize{ sizeof(vec2) * 6 };
    static constexpr size_t kVertexUvOffset{ kVertexPosSize };
    static constexpr size_t kVertexUvSize{ sizeof(vec2) * 6 };

    static constexpr size_t kElemDimSize{ sizeof(vec4) };
    static constexpr size_t kElemUvSize{ sizeof(vec4) };
    static constexpr size_t kElemColorSize{ sizeof(vec4) };

    static constexpr ui32 baseTextureIndex{ 0 };
    static constexpr ui32 fontTextureIndex{ 1 };

    struct DeviceData
    {
        struct DrawCmd
        {
            ui32 indexCount;
            ui32 instanceCount;

            ui32 textureIndex;
            trc::Pipeline::ID pipeline;
        };

        std::vector<DrawCmd> drawCmds;

        trc::Buffer dimensionBuf;
        trc::Buffer uvBuf;
        trc::Buffer colorBuf;

        vec4* dimensions;
        vec4* uvs;
        vec4* colors;
    };

    struct Texture
    {
        vk::UniqueImageView imageView;
        vk::UniqueSampler sampler;

        vk::UniqueDescriptorSet descSet;
    };

    static void ensureBufferSize(const GraphRenderData& renderData,
                                 DeviceData& deviceData,
                                 const trc::Device& device);

    const trc::Device& device;

    vk::UniqueDescriptorPool pool;
    vk::UniqueDescriptorSetLayout layout;
    trc::DescriptorProvider descProvider;

    trc::Image baseTexture;
    Texture textures[2];

    trc::DeviceLocalBuffer vertexBuf;
    trc::DeviceLocalBuffer indexBuf;
    trc::FrameSpecific<DeviceData> deviceData;
};
