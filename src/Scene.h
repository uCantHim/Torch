#pragma once
#ifndef SCENE_H
#define SCENE_H

#include <vector>

#include "VulkanDrawable.h"

class Scene
{
public:
    std::vector<VulkanDrawableInterface*> drawables;
};

#endif