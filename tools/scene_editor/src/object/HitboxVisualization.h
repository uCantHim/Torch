#pragma once

#include <trc/core/Pipeline.h>
#include <trc/core/SceneBase.h>

#include "Hitbox.h"

auto getHitboxPipeline() -> trc::Pipeline::ID;

auto makeHitboxDrawable(trc::SceneBase& scene, const Sphere& sphere)
    -> trc::MaybeUniqueRegistrationId;
auto makeHitboxDrawable(trc::SceneBase& scene, const Capsule& caps)
    -> trc::MaybeUniqueRegistrationId;
