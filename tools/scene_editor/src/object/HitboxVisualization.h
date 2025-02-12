#pragma once

#include <array>

#include <trc/Torch.h>
#include <trc/drawable/DrawableScene.h>

#include "scene/Geometry.h"

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

    void enableBox(const Box& box);
    void disableBox();
    bool isBoxEnabled() const;

private:
    trc::Scene* scene;

    trc::Drawable sphereDrawable;
    trc::Drawable boxDrawable;
    std::array<trc::Drawable, 3> capsuleDrawables;
};
