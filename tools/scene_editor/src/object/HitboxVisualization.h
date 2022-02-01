#pragma once

#include <trc/core/Pipeline.h>
#include <trc/drawable/Drawable.h>

#include "Hitbox.h"

class AssetManager;

/**
 * The capsule visualization is a stretched sphere until I implement a
 * good capsule geometry
 */
class HitboxVisualization : public trc::Node
{
public:
    HitboxVisualization() = default;

    void attachToScene(trc::SceneBase& scene);
    void removeFromScene();

    void enableSphere(const Sphere& sphere);
    void disableSphere();
    bool isSphereEnabled() const;

    void enableCapsule(const Capsule& capsule);
    void disableCapsule();
    bool isCapsuleEnabled() const;

private:
    trc::SceneBase* scene{ nullptr };

    bool showSphere{ false };
    bool showCapsule{ false };
    trc::Drawable sphereDrawable;
    trc::Drawable capsuleDrawable;
};

template<>
struct componentlib::TableTraits<HitboxVisualization>
{
    using UniqueStorage = std::true_type;
};
