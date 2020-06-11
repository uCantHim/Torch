#include <iostream>

#include "Scene.h"
#include "StaticDrawable.h"

/**
 * @brief A sample implementation
 */
class Thing : public SceneRegisterable,
              public StaticPipelineRenderInterface<Thing, 0, 0>,
              public StaticPipelineRenderInterface<Thing, 0, 1>
{
public:
    // template<GraphicsPipeline::ID Pipeline>
    // void recordCommandBuffer(vk::CommandBuffer)
    // {
    //     if constexpr (Pipeline == 0)
    //     {
    //         // ...
    //     }
    //     else if constexpr (Pipeline == 1)
    //     {
    //         // ...
    //     }
    // }


    void recordCommandBuffer(PipelineIndex<0>, vk::CommandBuffer)
    {

    }

    void recordCommandBuffer(PipelineIndex<1>, vk::CommandBuffer)
    {

    }
};

int main()
{
    Scene scene;

    scene.registerDrawFunction(0, 4, [](vk::CommandBuffer) {
        std::cout << "Draw function with pipeline 4 invoked\n";
    });

    Thing thing;
    thing.attachToScene(scene);
    thing.removeFromScene();

    std::cout << " --- Done\n";
    return 0;
}
