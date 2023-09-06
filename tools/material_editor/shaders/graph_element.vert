#version 460

layout (location = 0) in vec2 vertexPosition;
layout (location = 1) in vec2 vertexUv;
layout (location = 2) in vec4 itemDims;
layout (location = 3) in vec4 itemUvDims;
layout (location = 4) in vec4 itemColor;

layout (set = 0, binding = 0) uniform CameraData
{
    mat4 viewMatrix;
    mat4 projMatrix;
    //mat4 inverseViewMatrix;
    //mat4 inverseProjMatrix;
};

layout (location = 0) out PerVertex
{
    vec2 uv;
    vec4 color;
} vert;

void main()
{
    const vec2 itemPos = itemDims.xy;
    const vec2 itemSize = itemDims.zw;
    const vec2 worldPos = itemPos + itemSize * vertexPosition;

    const vec2 itemUvOffset = itemUvDims.xy;
    const vec2 itemUvSize = itemUvDims.zw;
    const vec2 uv = itemUvOffset + itemUvSize * vertexUv;

    gl_Position = projMatrix * viewMatrix * vec4(worldPos, 0.0f, 1.0f);
    vert.uv = uv;
    vert.color = itemColor;
}
