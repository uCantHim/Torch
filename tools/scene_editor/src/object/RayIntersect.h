#include <optional>

#include "scene/Geometry.h"
#include "util/Math.h"

struct Ray
{
    vec3 origin;
    vec3 direction;

    //float minT{ 0.0f };
    //float maxT{ 100.0f };
};

struct Intersection
{
    vec3 hitPoint;
    float t;
};

/**
 * @brief Ray-sphere intersection
 *
 * @return The point on the ray which is closest to the sphere's center, if the
 *         ray intersects the sphere. Nothing otherwise.
 */
inline constexpr
auto intersect(const Ray& ray, const Sphere& sphere) -> std::optional<Intersection>
{
    const float minLambda = glm::max(0.0f,
        -(glm::dot(ray.direction, (ray.origin - sphere.position))
          / glm::dot(ray.direction, ray.direction))
    );

    const vec3 minDistPoint = ray.origin + minLambda * ray.direction;
    const float minDist2 = util::distance2(minDistPoint, sphere.position);

    //const float minDist2 =
    //    minLambda * minLambda * glm::dot(ray.direction, ray.direction)
    //    + minLambda * 2.0f * glm::dot(ray.direction, ray.origin - sphere.position)
    //    + glm::dot(ray.origin - sphere.position, ray.origin - sphere.position);

    if (minDist2 < (sphere.radius * sphere.radius)) {
        return Intersection{ minDistPoint, minLambda };
    }
    return std::nullopt;
}

/**
 * @brief Ray-sphere intersection at the sphere's boundaries
 */
inline constexpr
auto intersectEdge(const Ray& ray, const Sphere& sphere)
    -> std::optional<std::pair<Intersection, Intersection>>
{
    // It's not useful to try to de-duplicate recurring expressions like
    // dot(v, o-p), even though it may look tempting. At optimization level -O2,
    // both gcc and clang remove all of the duplication on their own.
    //
    // Tested with goldbolt.org

    const vec3 o = ray.origin;
    const vec3 v = ray.direction;
    const vec3 p = sphere.position;
    const float r = sphere.radius;

    // About the last expression `(r * r)`:
    //
    // This is the sphere's radius for which we solve, and it must be squared
    // because this formula calculates zeroes for the *squared distance*
    // function between ray and sphere: d^2(lambda) = r^2
    const float sqrt_arg = 4.0f * glm::dot(v, o - p) * glm::dot(v, o - p)
                           - 4.0f * glm::dot(v, v) * (glm::dot(o - p, o - p) - (r * r));
    if (sqrt_arg < 0.0f) {
        return std::nullopt;  // The root is complex
    }

    const float _sqrt = util::sqrt(sqrt_arg);
    const float lambda1 =
        (-2.0f * glm::dot(v, o - p) + _sqrt)
        / 2.0f * glm::dot(v, v);
    const float lambda2 =
        (-2.0f * glm::dot(v, o - p) - _sqrt)
        / 2.0f * glm::dot(v, v);

    const vec3 intersect1 = o + lambda1 * v;
    const vec3 intersect2 = o + lambda2 * v;

    // Order points by distance from the ray's origin.
    if (lambda1 < lambda2) {
        return std::make_pair(Intersection{ intersect1, lambda1 },
                              Intersection{ intersect2, lambda2 });
    }
    else {
        return std::make_pair(Intersection{ intersect2, lambda2 },
                              Intersection{ intersect1, lambda1 });
    }
}

inline constexpr
auto intersect(const Ray&, const Capsule&)
    -> std::optional<std::pair<Intersection, Intersection>>
{
    return std::nullopt;
}

static_assert(intersect({ vec3(0), vec3(1, 0, 0) }, { 0.5f, vec3(0) }));
static_assert(!intersect({ vec3(0), vec3(1, 0, 0) }, { 0.5f, vec3(1, 1, 1) }));
static_assert(intersect({ vec3(0, 0.4f, 0), vec3(1, 0, 0) }, { 0.5f, vec3(0) }));
static_assert(!intersect({ vec3(0, 0.6f, 0), vec3(1, 0, 0) }, { 0.5f, vec3(0) }));
static_assert(intersect({ vec3(0, 0.499f, 0), vec3(1, 0, 0) }, { 0.5f, vec3(10, 0, 0) }));
static_assert(!intersect({ vec3(0, 0.501f, 0), vec3(1, 0, 0) }, { 0.5f, vec3(10, 0, 0) }));

static_assert(intersectEdge({ vec3(0), vec3(1, 0, 0) }, { 0.5f, vec3(0) }));
static_assert(!intersectEdge({ vec3(0), vec3(1, 0, 0) }, { 0.5f, vec3(1, 1, 1) }));
static_assert(intersectEdge({ vec3(0, 0.4f, 0), vec3(1, 0, 0) }, { 0.5f, vec3(0) }));
static_assert(!intersectEdge({ vec3(0, 0.6f, 0), vec3(1, 0, 0) }, { 0.5f, vec3(0) }));
static_assert(intersectEdge({ vec3(0, 0.4f, 0), vec3(1, 0, 0) }, { 0.5f, vec3(10, 0, 0) }));
static_assert(!intersectEdge({ vec3(0, 0.501f, 0), vec3(1, 0, 0) }, { 0.5f, vec3(10, 0, 0) }));
