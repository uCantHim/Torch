#pragma once

#include <functional>

#include "SceneObject.h"

class Scene;

auto makeContext(Scene& scene, SceneObject obj) -> std::function<void()>;
