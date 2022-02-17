#pragma once

#include <componentlib/Table.h>

#include "AnimationEngine.h"

namespace trc::drawcomp
{
    struct AnimationComponent
    {
        AnimationComponent(Rig& rig) : rig(&rig), engine(rig) {}

        Rig* rig;
        AnimationEngine engine;
    };
} // namespace trc::drawcomp

/**
 * Because a pointer to the AnimationEngine will probably be exposed to the
 * user
 */
template<>
struct componentlib::TableTraits<trc::drawcomp::AnimationComponent>
{
    using UniqueStorage = std::true_type;
};
