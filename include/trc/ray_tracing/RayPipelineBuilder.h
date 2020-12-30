#pragma once

#include <string>
#include <filesystem>
namespace fs = std::filesystem;

#include "Types.h"
#include "ray_tracing/ShaderBindingTable.h"

namespace trc::rt
{
    class RayTracingPipelineBuilder
    {
    public:
        using UniquePipeline = vk::UniqueHandle<vk::Pipeline, vk::DispatchLoaderDynamic>;
        using Self = RayTracingPipelineBuilder;

        auto addRaygenGroup(const fs::path& raygenPath) -> Self&;
        auto addMissGroup(const fs::path& missPath) -> Self&;
        auto addTrianglesHitGroup(const fs::path& closestHitPath,
                                  const fs::path& anyHitPath) -> Self&;
        auto addProceduralHitGroup(const fs::path& intersectionPath,
                                   const fs::path& closestHitPath,
                                   const fs::path& anyHitPath) -> Self&;
        auto addCallableGroup(const fs::path& callablePath) -> Self&;

        auto build(ui32 maxRecursionDepth, vk::PipelineLayout layout)
            -> std::pair<UniquePipeline, ShaderBindingTable>;

    private:
        auto addShaderModule(const fs::path& path) -> vk::ShaderModule;
        auto addPipelineStage(vk::ShaderModule _module, vk::ShaderStageFlagBits stage) -> ui32;

        // Need to be kept alive for the stage create infos
        std::vector<vk::UniqueShaderModule> shaderModules;

        std::vector<vk::PipelineShaderStageCreateInfo> pipelineStages;
        std::vector<vk::RayTracingShaderGroupCreateInfoKHR> shaderGroups;
    };

	auto inline _buildRayTracingPipeline() -> RayTracingPipelineBuilder
	{
		return RayTracingPipelineBuilder{};
	}
} // namespace trc::rt
