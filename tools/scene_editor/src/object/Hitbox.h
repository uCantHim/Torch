#pragma once

#include <optional>

#include <trc/Types.h>
#include <trc/assets/Geometry.h>
using namespace trc::basic_types;

#include "scene/Geometry.h"
#include "scene/RayIntersect.h"

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

    /**
     * @brief Test if a ray intersects with the hitbox
     *
     * @return The intersection nearest to the ray origin, if one exists.
     *         Nothing otherwise.
     */
    auto intersect(const Ray& ray) const -> std::optional<Intersection>;

private:
    Sphere sphere;
    Capsule capsule;
};

auto makeHitbox(const trc::GeometryData& geo) -> Hitbox;
