#pragma once

#include <vector>
#include <functional>
#include <variant>

#include <vkb/basics/Swapchain.h>

#include "Types.h"
#include "Transform.h"

namespace trc::ui
{
    /**
     * Idea:
     *
     * Global positions don't exist. They are calculated exclusively for
     * internal calculations during drawing.
     */
    class Node
    {
    public:
        auto getPos() -> SizeVal;
        auto getSize() -> SizeVal;
        void setPos(SizeVal newPos);
        void setSize(SizeVal newSize);

        auto getPositionProperties() -> Transform::Properties;
        auto getSizeProperties() -> Transform::Properties;
        auto setPositionProperties(Transform::Properties newProps);
        auto setSizeProperties(Transform::Properties newProps);

        auto getTransform() -> Transform;
        void setTransform(Transform newTransform);

        void attach(Node& child);
        void detach(Node& child);

    private:
        Transform localTransform;

        Node* parent { nullptr };
        std::vector<Node*> children;
    };

    class Element : public Node
    {
    public:
    };

    template<typename T>
    concept GuiElement = requires {
        std::is_base_of_v<Element, T>;
    };
} // namespace trc::ui
