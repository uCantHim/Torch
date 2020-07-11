#pragma once

#include "SceneRegisterable.h"

class Geometry;
class Material;

/**
 * @brief Purely component-based Drawable class
 */
class DrawableBase : public SceneRegisterable
{
public:
    auto getGeometry();
    auto getMaterial();

    // Look the transformation up in a global array of matrices
    // I can keep a separate array of links to parents
    auto getTransform();

protected:
    Geometry* geo;
    Material* material;
};
