#pragma once

#include <componentlib/Table.h>

#include "trc/AnimationEngine.h"
#include "trc/assets/Rig.h"

namespace trc
{
    struct AnimationComponent
    {
        explicit AnimationComponent(RigID rig)
            :
            rig(rig.getDeviceDataHandle()),
            engine(this->rig)
        {}

        RigHandle rig;
        AnimationEngine engine;
    };
} // namespace trc::drawcomp

/**
 * Because a pointer to the AnimationEngine will probably be exposed to the
 * user
 */
template<>
struct componentlib::TableTraits<trc::AnimationComponent>
{
    using UniqueStorage = std::true_type;
};
