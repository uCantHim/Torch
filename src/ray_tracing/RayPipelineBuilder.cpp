#include "ray_tracing/RayPipelineBuilder.h"

#include <vkb/basics/Device.h>
#include <vkb/ShaderProgram.h>



trc::rt::RayTracingPipelineBuilder::RayTracingPipelineBuilder(const ::trc::Instance& instance)
    :
    device(instance.getDevice()),
    dl(instance.getDL())
{
}

auto trc::rt::RayTracingPipelineBuilder::beginTableEntry() -> Self&
{
    if (hasActiveEntry) {
        endTableEntry();
    }
    hasActiveEntry = true;

    return *this;
}

auto trc::rt::RayTracingPipelineBuilder::endTableEntry() -> Self&
{
    if (currentEntrySize != 0)
    {
        sbtEntries.push_back(currentEntrySize);
        currentEntrySize = 0;
    }
    hasActiveEntry = false;

    return *this;
}

auto trc::rt::RayTracingPipelineBuilder::addRaygenGroup(const fs::path& raygenPath) -> Self&
{
    auto& group = addShaderGroup(vk::RayTracingShaderGroupTypeKHR::eGeneral);

    auto newModule = addShaderModule(raygenPath);
    ui32 pipelineStageIndex = addPipelineStage(newModule, vk::ShaderStageFlagBits::eRaygenKHR);
    group.generalShader = pipelineStageIndex;

    return *this;
}

auto trc::rt::RayTracingPipelineBuilder::addMissGroup(const fs::path& missPath) -> Self&
{
    auto& group = addShaderGroup(vk::RayTracingShaderGroupTypeKHR::eGeneral);

    auto newModule = addShaderModule(missPath);
    ui32 pipelineStageIndex = addPipelineStage(newModule, vk::ShaderStageFlagBits::eMissKHR);
    group.generalShader = pipelineStageIndex;

    return *this;
}

auto trc::rt::RayTracingPipelineBuilder::addTrianglesHitGroup(
    const fs::path& closestHitPath,
    const fs::path& anyHitPath) -> Self&
{
    auto& group = addShaderGroup(vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup);

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
    auto& group = addShaderGroup(vk::RayTracingShaderGroupTypeKHR::eProceduralHitGroup);

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
    auto& group = addShaderGroup(vk::RayTracingShaderGroupTypeKHR::eGeneral);

    auto newModule = addShaderModule(callablePath);
    ui32 pipelineStageIndex = addPipelineStage(newModule, vk::ShaderStageFlagBits::eCallableKHR);
    group.generalShader = pipelineStageIndex;

    return *this;
}

auto trc::rt::RayTracingPipelineBuilder::build(
    ui32 maxRecursionDepth,
    PipelineLayout& layout,
    const vkb::DeviceMemoryAllocator alloc
    ) -> std::pair<Pipeline, ShaderBindingTable>
{
	assert(pipelineStages.size() > 0);

    // Create pipeline
	auto pipeline = device->createRayTracingPipelineKHRUnique(
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
			*layout
		),
		nullptr, dl
	).value;

    // Create shader binding table
    ShaderBindingTable sbt{ device, dl, *pipeline, sbtEntries, alloc };

    return {
        Pipeline(layout, std::move(pipeline), vk::PipelineBindPoint::eRayTracingKHR),
        std::move(sbt)
    };
}

auto trc::rt::RayTracingPipelineBuilder::addShaderModule(const fs::path& path) -> vk::ShaderModule
{
    if (!fs::is_regular_file(path)) {
        throw std::runtime_error(path.string() + " is not a regular file");
    }

    return *shaderModules.emplace_back(
        vkb::createShaderModule(device, vkb::readFile(path.string()))
    );
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

auto trc::rt::RayTracingPipelineBuilder::addShaderGroup(vk::RayTracingShaderGroupTypeKHR type)
    -> vk::RayTracingShaderGroupCreateInfoKHR&
{
    const bool isInEntry{ hasActiveEntry };
    // Create a new entry just for this group if no entry is active
    if (!isInEntry) {
        beginTableEntry();
    }

    auto& result = shaderGroups.emplace_back(
        vk::RayTracingShaderGroupCreateInfoKHR(
            type,
            VK_SHADER_UNUSED_KHR, // Index of general      shader in the pipelineStages array
            VK_SHADER_UNUSED_KHR, // Index of closest hit  shader in the pipelineStages array
            VK_SHADER_UNUSED_KHR, // Index of any hit      shader in the pipelineStages array
            VK_SHADER_UNUSED_KHR  // Index of intersection shader in the pipelineStages array
        )
    );
    currentEntrySize++;

    if (!isInEntry) {
        endTableEntry();
    }

    return result;
}
