#pragma once

#include <componentlib/ComponentBase.h>
#include <trc/Types.h>
#include <trc/Geometry.h>
using namespace trc::basic_types;

struct Capsule
{
    Capsule() = default;
    Capsule(float height, float radius, vec3 pos = vec3(0.0f))
        : height(height), radius(radius), position(pos)
    {}

    /** Full height from end to end; cylinder's height is height - 2 * radius. */
    float height{ 0.0f };
    float radius{ 0.0f };
    vec3 position;
};

struct Sphere
{
    Sphere() = default;
    Sphere(float radius, vec3 pos = vec3(0.0f))
        : radius(radius), position(pos)
    {}

    float radius{ 0.0f };
    vec3 position;
};

bool isInside(vec3 point, const Sphere& sphere);
bool isInside(vec3 point, const Capsule& capsule);

/**
 * @brief Each object has a spherical hitbox and a capsule hitbox
 */
class Hitbox
{
public:
    Hitbox(Sphere sphere, Capsule capsule);

    auto getSphere() const -> const Sphere&;
    auto getCapsule() const -> const Capsule&;

    /**
     * @brief Test if a point is inside of the hitbox
     *
     * The point has to be inside of the sphere as well as the capsule. The
     * sphere is used as a early-discard hitbox; the capsule is only tested
     * if the point lies inside of the sphere.
     */
    bool isInside(vec3 point) const;

private:
    Sphere sphere;
    Capsule capsule;
};

auto makeHitbox(const trc::GeometryData& geo) -> Hitbox;
