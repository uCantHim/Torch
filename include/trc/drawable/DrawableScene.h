#pragma once

#include <atomic>

#include <trc_util/data/SafeVector.h>

#include "trc/Types.h"
#include "trc/drawable/Drawable.h"
#include "trc/drawable/DrawableComponentScene.h"

namespace trc
{
    // struct DrawableHandle
    // {
    //     auto operator->() -> Drawable*;
    //     auto operator->() const -> const Drawable*;
    // };

    using DrawableHandle = s_ptr<Drawable>;

    class DrawableScene
    {
    public:
        explicit DrawableScene(SceneBase& base);

        void updateAnimations(float timeDelta) {
            components.updateAnimations(timeDelta);
        }
        void updateRayData() {
            components.updateRayData();
        }

        auto makeDrawable(const DrawableCreateInfo& createInfo) -> DrawableHandle;

        auto getDrawableInternals() const -> const DrawableComponentScene& {
            return components;
        }

    private:
        DrawableComponentScene components;

        util::SafeVector<Drawable> drawables;
    };
} // namespace trc
