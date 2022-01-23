#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <componentlib/ComponentID.h>
#include <componentlib/ComponentBase.h>
#include <trc/Node.h>
using namespace trc::basic_types;

struct _SceneObjectTypeTag {};
using SceneObject = componentlib::ComponentID<_SceneObjectTypeTag>;

struct ObjectBaseNode : componentlib::ComponentBase<ObjectBaseNode>
                      , trc::Node
{
};

template<>
struct componentlib::TableTraits<ObjectBaseNode>
{
    using UniqueStorage = std::true_type;
};
