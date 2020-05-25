#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec3 vertexNormal;
layout(location = 2) in vec2 vertexUvCoords;

layout(set = 0, binding = 0) uniform Matrices {
    mat4 model;
    mat4 view;
    mat4 proj;
} matrix;

layout (push_constant) uniform pushConstants {
    mat4 modelMatrix;
    mat4 viewMatrix;
    mat4 projMatrix;
};

out Vertex {
    layout(location = 0) out vec3 position;
    layout(location = 1) out vec3 normal;
    layout(location = 2) out vec2 uvCoords;
} vert;

void main()
{
    vert.position = vertexPosition;
    vert.normal = vertexNormal;
    vert.uvCoords = vertexUvCoords;

    gl_Position = matrix.proj * matrix.view * modelMatrix * vec4(vertexPosition, 1.0);
}
