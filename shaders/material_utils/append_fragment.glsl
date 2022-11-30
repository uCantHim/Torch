void appendFragment(vec4 color)
{
    uint newIndex = atomicAdd($nextFragmentListIndex, 1);
    if (newIndex > $maxFragmentListIndex) {
        return;
    }

    uvec4 newElement = uvec4(
        packUnorm4x8(color),
        floatBitsToUint(gl_FragCoord.z - 0.001),
        imageAtomicExchange($fragmentListHeadPointer, ivec2(gl_FragCoord.xy), newIndex),
        42
    );
    $fragmentList[newIndex] = newElement;
}
