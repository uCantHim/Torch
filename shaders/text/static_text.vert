#version 460

layout (location = 0) in vec3 vertexPosition;
layout (location = 1) in vec2 vertexUvCoord;

layout (location = 2) in vec2 texCoordLL;
layout (location = 3) in vec2 texCoordUR;
layout (location = 4) in vec2 glyphOffset;
layout (location = 5) in vec2 glyphSize;
layout (location = 6) in float bearingY;

layout (set = 0, binding = 0, std140) restrict uniform CameraBuffer
{
    mat4 viewMatrix;
    mat4 projMatrix;
    mat4 inverseViewMatrix;
    mat4 inverseProjMatrix;
} camera;

layout (push_constant) uniform PushConstants {
    // Model matrix of the whole text string
    mat4 modelMatrix;
    // Font's index in the glyph map descriptor
    uint textureIndex;
};

layout (location = 0) out Vertex {
    sample vec2 uv;
    flat uint textureIndex;
} vert;

void main()
{
    vec3 vertPos = vec3(glyphSize, 1.0) * vertexPosition
                   - vec3(0, bearingY, 0)
                   + vec3(glyphOffset, 0);
    gl_Position = camera.projMatrix * camera.viewMatrix * modelMatrix * vec4(vertPos, 1.0);

    vert.uv = texCoordLL + vertexUvCoord * (texCoordUR - texCoordLL);
    vert.textureIndex = textureIndex;
}
