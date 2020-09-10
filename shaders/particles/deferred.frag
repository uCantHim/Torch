#version 460
#extension GL_EXT_nonuniform_qualifier : require

layout (early_fragment_tests) in;

layout (set = 1, binding = 1) uniform sampler2D textures[];

//////////////////
// Transparency //
//////////////////

layout (set = 2, binding = 4, r32ui) uniform uimage2D fragmentListHeadPointer;

layout (set = 2, binding = 5) restrict buffer FragmentListAllocator
{
    uint nextFragmentListIndex;
    uint maxFragmentListIndex;
};

layout (set = 2, binding = 6) restrict buffer FragmentList
{
    /**
     * Four components because uvec3 causes alignment issues :/
     *
     * 0: A packed color
     * 1: Fragment depth value
     * 2: Next-pointer
     */
    uvec4 fragmentList[];
};

void appendFragment(vec4 color)
{
    uint newIndex = atomicAdd(nextFragmentListIndex, 1);
    if (newIndex > maxFragmentListIndex) {
        return;
    }

    uvec4 newElement = uvec4(
        packUnorm4x8(color),
        floatBitsToUint(gl_FragCoord.z - 0.001),
        imageAtomicExchange(fragmentListHeadPointer, ivec2(gl_FragCoord.xy), newIndex),
        42
    );
    fragmentList[newIndex] = newElement;
}

///////////////////
//      Main     //
///////////////////

layout (location = 0) in Vertex
{
    vec2 uv;
    flat uint textureIndex;
} vert;

void main()
{
    vec4 diffuseColor = texture(textures[vert.textureIndex], vert.uv);
    if (diffuseColor.a > 0.001) {
        appendFragment(diffuseColor);
    }
}
