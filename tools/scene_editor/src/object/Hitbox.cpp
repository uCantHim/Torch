#include "Hitbox.h"

#include <limits>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>
#include <glm/gtx/vec_swizzle.hpp>
#include <trc/base/Logging.h>



auto makeHitbox(const trc::GeometryData& geo) -> Hitbox
{
    vec3 maxCoords{ std::numeric_limits<float>::min() };
    vec3 minCoords{ std::numeric_limits<float>::max() };

    for (const trc::MeshVertex& vert : geo.vertices)
    {
        maxCoords = max(vert.position, maxCoords);
        minCoords = min(vert.position, minCoords);
    }

    const vec3 midPoint = (maxCoords + minCoords) * 0.5f;
    const vec3 maxAbsCoords = max(maxCoords, abs(minCoords));
    const float radius = distance(midPoint, maxAbsCoords);
    Sphere sphere(radius, midPoint);

    const vec3 lowerPoint = vec3(midPoint.x, minCoords.y, midPoint.z);
    const float height = maxCoords.y - minCoords.y;
    const float xzRadius = distance(xz(lowerPoint), xz(maxAbsCoords));
    Capsule capsule(height, xzRadius, midPoint);

    // Logging
    {
        vec3 m = sphere.position;
        trc::log::info << "Generated hitbox for geometry with "
            << "sphere [m = (" << m.x << ", " << m.y << ", " << m.z << "), r = " << sphere.radius
            << "] and capsule [r = " << capsule.radius << ", h = " << capsule.height << "]"
            << "\n";
    }

    return { sphere, capsule };
}



bool isInside(vec3 point, const Sphere& sphere)
{
    return distance2(point, sphere.position) < sphere.radius * sphere.radius;
}

bool isInside(vec3 p, const Capsule& cap)
{
    const vec3 a = vec3(0.0f, cap.radius, 0.0f) + cap.position;
    const vec3 b = vec3(0.0f, cap.height - 2 * cap.radius, 0.0f);

    const float maxDist2 = cap.radius * cap.radius;

    // Test the spheres on both ends first
    if (distance2(p, a) < maxDist2 || distance2(p, a + b) < maxDist2) {
        return true;
    }

    const float r = -dot(b, p - a) / dot(b, b);
    return
        r > 0.0f && r < 1.0f  // r is in the valid line segment
        && maxDist2 > distance2(a + r * b, p);  // Shortest dist to p is smaller than radius
}



Hitbox::Hitbox(Sphere sphere, Capsule capsule)
    :
    sphere(sphere),
    capsule(capsule)
{
}

auto Hitbox::getSphere() const -> const Sphere&
{
    return sphere;
}

auto Hitbox::getCapsule() const -> const Capsule&
{
    return capsule;
}

bool Hitbox::isInside(vec3 point) const
{
    return ::isInside(point, sphere) && ::isInside(point, capsule);
}

auto Hitbox::intersect(const Ray& ray) const -> std::optional<Intersection>
{
    if (auto hit = intersectEdge(ray, sphere)) {
        return hit->first;
    }
    return std::nullopt;
}
