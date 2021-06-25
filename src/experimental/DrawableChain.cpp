#include "drawable/DrawableChain.h"



namespace trc
{
    void DrawableChainElement::attachToScene(SceneBase& scene)
    {
        // This can only be the case when *this was moved from
        if (drawable) {
            drawable->attachToScene(scene);
        }
        if (nextDecorated) {
            nextDecorated->attachToScene(scene);
        }
    }

    void DrawableChainElement::removeFromScene()
    {
        // This can only be the case when *this was moved from
        if (drawable) {
            drawable->removeFromScene();
        }
        if (nextDecorated) {
            nextDecorated->removeFromScene();
        }
    }

    auto DrawableChainElement::getDrawable() noexcept -> DrawableInterface&
    {
        return *drawable;
    }

    auto DrawableChainElement::getDrawable() const noexcept -> const DrawableInterface&
    {
        return *drawable;
    }

    auto DrawableChainElement::getNextChainElement() noexcept
        -> Maybe<DrawableChainElement&>
    {
        if (nextDecorated != nullptr) {
            return *nextDecorated;
        }

        return {};
    }

    auto DrawableChainElement::getNextChainElement() const noexcept
        -> Maybe<const DrawableChainElement&>
    {
        if (nextDecorated != nullptr) {
            return *nextDecorated;
        }

        return {};
    }
} // namespace trc::experimental
