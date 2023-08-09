#include <glm/glm.hpp>
#include <trc/assets/Geometry.h>

using namespace trc::basic_types;

inline auto maxOf(vec3 v) -> float
{
    return glm::max(glm::max(v.x, v.y), v.z);
}

inline auto calcExtent(const trc::GeometryData& geo) -> vec3
{
    vec3 r{ 0.0f };
    for (const auto& v : geo.vertices) {
        r = glm::max(r, glm::abs(v.position));
    }

    return r;
}
