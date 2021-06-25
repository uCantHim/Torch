#pragma once

#include <nc/data/SelfManagedObject.h>

#include "Types.h"

namespace trc
{
    struct RenderStageType : public data::SelfManagedObject<RenderStageType>
    {
    public:
        RenderStageType(ui32 numSubPasses) : numSubPasses(numSubPasses) {}

        const ui32 numSubPasses;
    };
} // namespace trc
