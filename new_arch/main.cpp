#include <iostream>

#include "Scene.h"

int main()
{
    Scene scene;

    scene.registerDrawFunction(0, 4, [](vk::CommandBuffer) {
        std::cout << "Draw function with pipeline 4 invoked\n";
    });

    std::cout << " --- Done\n";
    return 0;
}
