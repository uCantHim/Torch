#version 460

//$ vertex_location
in vec3 vertexPosition;

layout (push_constant) uniform PushConstants {
    mat4 model;
}

void main()
{
    gl_Position = model * vec4(vertexPosition, 1.0);
}
