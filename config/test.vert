#version 460

layout (location = 0) in vec3 vertexPosition;

layout (push_constant) uniform PushConstants
{
    mat4 model;
    mat4 view;
    mat4 proj;
};

vec3 applyAnimation(vec3 vertex)
{
    return vertex;
}

void main()
{
    vec4 worldPos = model * vec4(vertexPosition, 1.0);

    //$ Animation

    //$ ViewTransform

    gl_Position = proj * worldPos;
}
