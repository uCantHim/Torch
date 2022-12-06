#pragma once

#include <trc/drawable/DrawableComponentScene.h>

#include "Hitbox.h"

namespace trc{
    class Scene;
}

/**
 * The capsule visualization is a stretched sphere until I implement a
 * good capsule geometry
 */
class HitboxVisualization : public trc::Node
{
public:
    HitboxVisualization(trc::Scene& scene);

    void removeFromScene();

    void enableSphere(const Sphere& sphere);
    void disableSphere();
    bool isSphereEnabled() const;

    void enableCapsule(const Capsule& capsule);
    void disableCapsule();
    bool isCapsuleEnabled() const;

private:
    trc::Scene* scene;

    bool showSphere{ false };
    bool showCapsule{ false };
    trc::UniqueDrawableID sphereDrawable;
    trc::UniqueDrawableID capsuleDrawable;
};

template<>
struct componentlib::TableTraits<HitboxVisualization>
{
    using UniqueStorage = std::true_type;
};
