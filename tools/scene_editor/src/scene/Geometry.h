#pragma once

#include <trc/Types.h>
using namespace trc::basic_types;

struct Capsule
{
    constexpr Capsule() = default;
    constexpr Capsule(float height, float radius, vec3 pos = vec3(0.0f))
        : height(height), radius(radius), position(pos)
    {}

    // Height of the cylinder, i.e., the capsule's height excluding both ends.
    float height{ 0.0f };
    float radius{ 0.0f };
    vec3 position;
};

struct Sphere
{
    constexpr Sphere() = default;
    constexpr Sphere(float radius, vec3 pos = vec3(0.0f))
        : radius(radius), position(pos)
    {}

    float radius{ 0.0f };
    vec3 position;
};

struct Box
{
    constexpr Box() = default;
    constexpr Box(vec3 halfExtent, vec3 pos = vec3(0.0f))
        : halfExtent(halfExtent), position(pos)
    {}

    vec3 halfExtent;
    vec3 position;
};

