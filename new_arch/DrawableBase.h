#pragma once

#include "base/SceneRegisterable.h"
#include "Node.h"

class Geometry;
class Material;

/**
 * @brief Purely component-based Drawable class
 */
class DrawableBase : public SceneRegisterable, public Node
{
public:
    auto getGeometry();
    auto getMaterial();

protected:
    Geometry* geo;
    Material* material;
};
