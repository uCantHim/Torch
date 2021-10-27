#include <iostream>

#include <glm/gtc/random.hpp>
#include <vkb/util/Timer.h>
#include <trc/Torch.h>
#include <trc/ray_tracing/RayTracing.h>
#include <trc/drawable/Drawable.h>

namespace trc {
    using namespace legacy;
}
using namespace trc::basic_types;

void run()
{
    constexpr ui32 measuredFrames = 10000;
    constexpr ui32 numInstances = 8000;
    constexpr ui32 numMemoryShuffles = numInstances * 3;

    srand(time(0));  // Init glm's random functions

    auto torch = trc::initFull();
    auto& ar = *torch.assetRegistry;

    // Create assets
    auto cubeGeo = ar.add(trc::makeCubeGeo());
    std::vector<trc::MaterialID> mats;
    for (int i = 0; i < 20; i++)
    {
        vec4 color(glm::linearRand(vec3(0), vec3(1)), 1.0f);
        mats.emplace_back(ar.add(trc::Material{ .color=color }));
    }
    ar.updateMaterials();

    // Camera
    const vec2 size = torch.window->getSwapchain().getSize();
    trc::Camera camera(size.x / size.y, 60.0f, 0.1f, 50.0f);
    camera.lookAt(vec3(15, 12, 13), vec3(0, 0, 0), vec3(0, 1, 0));

    // Create scenes
    trc::Scene poolScene;
    trc::DrawablePool pool(*torch.instance, { numInstances, false }, poolScene);
    std::vector<trc::DrawablePool::Handle> handles;

    trc::Scene drawScene;
    std::vector<trc::Drawable> drawables;

    poolScene.getLights().makeSunLight(vec3(1.0f), vec3(1, -1, -1), 0.6f);
    drawScene.getLights().makeSunLight(vec3(1.0f), vec3(1, -1, -1), 0.6f);

    // Create drawables
    auto base = pool.create({ cubeGeo, mats.at(glm::linearRand(0ul, mats.size() - 1ul)) });
    base->setTranslation(glm::linearRand(vec3(-10), vec3(10)));
    for (ui32 i = 0; i < numInstances; i++)
    {
        const vec3 pos = glm::linearRand(vec3(-10), vec3(10));
        const trc::MaterialID mat = mats.at(glm::linearRand(0ul, mats.size() - 1ul));

        auto inst = base->copy();
        inst->setTranslation(pos);
        handles.emplace_back(inst);

        auto& d = drawables.emplace_back(cubeGeo, mat, drawScene);
        d.setTranslation(pos);
    }

    // Shuffle objects around in memory to simulate real-world application
    for (int i = 0; i < numMemoryShuffles; i++)
    {
        const ui32 index = glm::linearRand(0u, numInstances - 1u);
        const vec3 pos = drawables[index].getTranslation();
        const auto mat = drawables[index].getMaterial();

        handles[index]->destroy();
        base->copy()->setTranslation(pos);

        drawables[index] = trc::Drawable(cubeGeo, mat, drawScene);
        drawables[index].setTranslation(pos);
    }


    // Draw pooled scene structure first
    vkb::Timer time;

    for (ui32 i = 0; i < measuredFrames; i++)
    {
        torch.drawFrame(torch.makeDrawConfig(poolScene, camera));
    }
    const float poolTime = time.reset();

    // Draw traditional functional scene
    time.reset();
    for (ui32 i = 0; i < measuredFrames; i++)
    {
        torch.drawFrame(torch.makeDrawConfig(drawScene, camera));
    }
    const float drawTime = time.reset();


    std::cout << measuredFrames << " frames with drawable pool: " << poolTime << "ms"
        << "\t\t(" << poolTime / measuredFrames << "ms avg frame time\n";
    std::cout << measuredFrames << " frames with traditional drawable: " << drawTime << "ms"
        << "\t(" << drawTime / measuredFrames << "ms avg frame time\n";

    torch.instance->getDevice()->waitIdle();
}

int main()
{
    run();
    trc::terminate();

    return 0;
}
