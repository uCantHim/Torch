#pragma once

#include <string>
#include <type_traits>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <componentlib/ComponentID.h>
#include <componentlib/ComponentBase.h>
#include <trc/Node.h>
using namespace trc::basic_types;

struct _SceneObjectTypeTag {};
using SceneObject = componentlib::ComponentID<_SceneObjectTypeTag>;

struct ObjectMetadata
{
    SceneObject id;
    std::string name;
};

/**
 * @brief The basic node component for any object with a transformation
 */
struct ObjectBaseNode : componentlib::ComponentBase<ObjectBaseNode>
                      , trc::Node
{
};
