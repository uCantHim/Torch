#include "ray_tracing/RayPipelineBuilder.h"

#include <vkb/basics/Device.h>
#include <vkb/ShaderProgram.h>



auto trc::rt::RayTracingPipelineBuilder::addRaygenGroup(const fs::path& raygenPath) -> Self&
{
    auto newModule = addShaderModule(raygenPath);
    ui32 pipelineStageIndex = addPipelineStage(newModule, vk::ShaderStageFlagBits::eRaygenKHR);
    shaderGroups.push_back(
        vk::RayTracingShaderGroupCreateInfoKHR(
            vk::RayTracingShaderGroupTypeKHR::eGeneral,
            pipelineStageIndex,   // Index of general      shader in the pipelineStages array
            VK_SHADER_UNUSED_KHR, // Index of closest hit  shader in the pipelineStages array
            VK_SHADER_UNUSED_KHR, // Index of any hit      shader in the pipelineStages array
            VK_SHADER_UNUSED_KHR  // Index of intersection shader in the pipelineStages array
        )
    );

    return *this;
}

auto trc::rt::RayTracingPipelineBuilder::addMissGroup(const fs::path& missPath) -> Self&
{
    auto newModule = addShaderModule(missPath);
    ui32 pipelineStageIndex = addPipelineStage(newModule, vk::ShaderStageFlagBits::eMissKHR);
    shaderGroups.push_back(
        vk::RayTracingShaderGroupCreateInfoKHR(
            vk::RayTracingShaderGroupTypeKHR::eGeneral,
            pipelineStageIndex,   // Index of general      shader in the pipelineStages array
            VK_SHADER_UNUSED_KHR, // Index of closest hit  shader in the pipelineStages array
            VK_SHADER_UNUSED_KHR, // Index of any hit      shader in the pipelineStages array
            VK_SHADER_UNUSED_KHR  // Index of intersection shader in the pipelineStages array
        )
    );

    return *this;
}

auto trc::rt::RayTracingPipelineBuilder::addTrianglesHitGroup(
    const fs::path& closestHitPath,
    const fs::path& anyHitPath) -> Self&
{
    auto& group = shaderGroups.emplace_back(
        vk::RayTracingShaderGroupCreateInfoKHR(
            vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup,
            VK_SHADER_UNUSED_KHR, // Index of general      shader in the pipelineStages array
            VK_SHADER_UNUSED_KHR, // Index of closest hit  shader in the pipelineStages array
            VK_SHADER_UNUSED_KHR, // Index of any hit      shader in the pipelineStages array
            VK_SHADER_UNUSED_KHR  // Index of intersection shader in the pipelineStages array
        )
    );

    if (!closestHitPath.empty())
    {
        auto chitModule = addShaderModule(closestHitPath);
        group.closestHitShader = addPipelineStage(
            chitModule,
            vk::ShaderStageFlagBits::eClosestHitKHR
        );
    }
    if (!anyHitPath.empty())
    {
        auto ahitModule = addShaderModule(anyHitPath);
        group.anyHitShader = addPipelineStage(
            ahitModule,
            vk::ShaderStageFlagBits::eAnyHitKHR
        );
    }

    return *this;
}

auto trc::rt::RayTracingPipelineBuilder::addProceduralHitGroup(
    const fs::path& intersectionPath,
    const fs::path& closestHitPath,
    const fs::path& anyHitPath) -> Self&
{
    auto& group = shaderGroups.emplace_back(
        vk::RayTracingShaderGroupCreateInfoKHR(
            vk::RayTracingShaderGroupTypeKHR::eProceduralHitGroup,
            VK_SHADER_UNUSED_KHR, // Index of general      shader in the pipelineStages array
            VK_SHADER_UNUSED_KHR, // Index of closest hit  shader in the pipelineStages array
            VK_SHADER_UNUSED_KHR, // Index of any hit      shader in the pipelineStages array
            VK_SHADER_UNUSED_KHR  // Index of intersection shader in the pipelineStages array
        )
    );

    if (!intersectionPath.empty())
    {
        auto intersectionModule = addShaderModule(intersectionPath);
        group.intersectionShader = addPipelineStage(
            intersectionModule,
            vk::ShaderStageFlagBits::eIntersectionKHR
        );
    }
    if (!closestHitPath.empty())
    {
        auto chitModule = addShaderModule(closestHitPath);
        group.closestHitShader = addPipelineStage(
            chitModule,
            vk::ShaderStageFlagBits::eClosestHitKHR
        );
    }
    if (!anyHitPath.empty())
    {
        auto ahitModule = addShaderModule(anyHitPath);
        group.anyHitShader = addPipelineStage(
            ahitModule,
            vk::ShaderStageFlagBits::eAnyHitKHR
        );
    }

    return *this;
}

auto trc::rt::RayTracingPipelineBuilder::addCallableGroup(const fs::path& callablePath) -> Self&
{
    auto newModule = addShaderModule(callablePath);
    ui32 pipelineStageIndex = addPipelineStage(newModule, vk::ShaderStageFlagBits::eCallableKHR);
    shaderGroups.push_back(
        vk::RayTracingShaderGroupCreateInfoKHR(
            vk::RayTracingShaderGroupTypeKHR::eGeneral,
            pipelineStageIndex,   // Index of general      shader in the pipelineStages array
            VK_SHADER_UNUSED_KHR, // Index of closest hit  shader in the pipelineStages array
            VK_SHADER_UNUSED_KHR, // Index of any hit      shader in the pipelineStages array
            VK_SHADER_UNUSED_KHR  // Index of intersection shader in the pipelineStages array
        )
    );

    return *this;
}

auto trc::rt::RayTracingPipelineBuilder::build(ui32 maxRecursionDepth, vk::PipelineLayout layout)
    -> std::pair<UniquePipeline, ShaderBindingTable>
{
	assert(pipelineStages.size() > 0);

    // Create pipeline
	auto pipeline = vkb::getDevice()->createRayTracingPipelineKHRUnique(
        {}, // optional deferred operation
		{}, // cache
		vk::RayTracingPipelineCreateInfoKHR(
			vk::PipelineCreateFlags(),
			pipelineStages,
			shaderGroups,
			maxRecursionDepth, // max recursion depth
            nullptr, // pipeline library create info
            nullptr, // ray tracing pipeline interface create info
            nullptr, // dynamic state create info
			layout
		),
		nullptr, vkb::getDL()
	).value;

    // Create corresponding shader binding table
    std::vector<ui32> tableEntries;
    for (const auto& group : shaderGroups) {
        tableEntries.push_back(1);
    }
    ShaderBindingTable sbt{
        vkb::getDevice(),
        *pipeline,
        std::move(tableEntries)
    };

    return { std::move(pipeline), std::move(sbt) };
}

auto trc::rt::RayTracingPipelineBuilder::addShaderModule(const fs::path& path) -> vk::ShaderModule
{
    if (!fs::is_regular_file(path)) {
        throw std::runtime_error(path.string() + " is not a regular file");
    }

    return *shaderModules.emplace_back(vkb::createShaderModule(vkb::readFile(path.string())));
}

auto trc::rt::RayTracingPipelineBuilder::addPipelineStage(
    vk::ShaderModule _module,
    vk::ShaderStageFlagBits stage) -> ui32
{
    pipelineStages.emplace_back(
        vk::PipelineShaderStageCreateFlags(),
        stage,
        _module,
        "main"
    );

    return pipelineStages.size() - 1;
}
