#pragma once

#include <string>
#include <vector>

#include <vkb/ShaderProgram.h>

#include "Types.h"

namespace trc::rt
{
	enum class RayShaderStage
	{
		eRaygen, eMiss, eCallable,
		eAnyHit, eClosestHit,
		eIntersection
	};

	using RayShaderGroup = vk::RayTracingShaderGroupTypeKHR;
	using ShaderStage = std::pair<vk::UniqueShaderModule, vk::PipelineShaderStageCreateInfo>;

	// I would have put this into RayTracingPipelineBuilder but I'm
	// too lazy to implement move constructors for the template
	// instantiations.
	struct ShaderGroup
	{
		std::vector<ShaderStage> stages;
		std::array<uint32_t, 4> stageIndices{
			VK_SHADER_UNUSED_KHR,
			VK_SHADER_UNUSED_KHR,
			VK_SHADER_UNUSED_KHR,
			VK_SHADER_UNUSED_KHR,
		};
	};

	/**
	 * @brief Builder for ray tracing pipelines
	 *
     * The process:
     *
     * 1. Create a builder with trc::rt::buildRayTracingPipeline() or
     *    trc::rt::RayTracingPipelineBuilder's default constructor.
     *
     * 2. Add a shader group to the pipeline
     *
     * 3. Add required stages to the group
     *
     * 4. Goto 2. or to 5. if you're done
     *
     * 5. Call build() to create the pipeline with the specified groups
	 *
	 * The builder gives compile-time errors if shaders are added to an
	 * incompatible shader group.
	 *
	 * Shader stages in the shader binding table are indexed in the order
	 * that they are added to the pipeline. That means that the first
	 * shader stage added to the pipeline via addStage() has the index 0
	 * in the shader binding table.
	 *
	 * @tparam RayShaderGroup GroupState The first group to build. It is
	 * not necessary to add stages to groups. Empty groups will be
	 * discarded.
	 */
	template<RayShaderGroup GroupState>
	class RayTracingPipelineBuilder
	{
	private:
		// Friend all builder template instantiations
		template<RayShaderGroup>
		friend class RayTracingPipelineBuilder;

		// A type wrapper that allows me to specialize based on enums
		template<RayShaderStage T> struct StageType {};

	public:
		RayTracingPipelineBuilder() = default;
		template<RayShaderGroup OtherGroupState>
		RayTracingPipelineBuilder(RayTracingPipelineBuilder<OtherGroupState>&& other) noexcept;
		~RayTracingPipelineBuilder() = default;

		RayTracingPipelineBuilder(const RayTracingPipelineBuilder&) = delete;
		RayTracingPipelineBuilder& operator=(RayTracingPipelineBuilder&&) = delete;
		RayTracingPipelineBuilder& operator=(const RayTracingPipelineBuilder&) = delete;

		/**
		 * @brief Create a new shader group on the built pipeline object
		 *
		 * Submits the current shader group and starts the creation of a
		 * new shader group. The state of the builder is set to a new
		 * shader group of the specified type.
		 *
		 * If the current group is empty (has no stages added), it will be
		 * discarded.
		 *
		 * @tparam RayShaderGroup Type of the new shader group
		 *
		 * @return The new builder state
		 */
		template<RayShaderGroup Group>
		auto addGroup() -> RayTracingPipelineBuilder<Group>;

		/**
		 * @brief Add a stage to the current group
		 *
		 * Will give a compile-time error if the stage type isn't
		 * compatible with the current shader group type.
		 *
		 * @tparam RyShaderStage Type of the added stage
		 *
		 * @return The new builder state
		 *
		 * @throw std::runtime_error if a module for the same slot has
		 * already been added to the current group.
		 * Remember: raygen, miss, and callable shaders occupy the same
		 * slot. A group can only ever hold one of them.
		 */
		template<RayShaderStage Stage>
		auto addStage(const std::string& modulePath, const std::string& entryPoint = "main")
			-> RayTracingPipelineBuilder<GroupState>;

		/**
		 * @brief Create the pipeline
		 *
		 * @param uint32_t maxRecursionDepth
		 * @param vk::PipelineLayout layout
		 */
		auto build(uint32_t maxRecursionDepth, vk::PipelineLayout layout) noexcept;

	private:
		// The following specializations are used to give compile-time
		// errors when stages are added to the wrong groups
		// I need the weird StageType<Stage> overload to be able to
		// specialize the member function
		template<RayShaderStage Stage>
		auto addStage(StageType<Stage>, const std::string& modulePath, const std::string& entryPoint) -> RayTracingPipelineBuilder<GroupState>;

		auto addStage(StageType<RayShaderStage::eRaygen>, const std::string& modulePath, const std::string& entryPoint)
			-> RayTracingPipelineBuilder<GroupState>
		{
			static_assert(GroupState == RayShaderGroup::eGeneral,
						  "Raygen shaders can only be added to general shader groups.");
			addStageToGroup<vk::ShaderStageFlagBits::eRaygenKHR>(modulePath, entryPoint);
			return std::move(*this);
		}

		auto addStage(StageType<RayShaderStage::eMiss>, const std::string& modulePath, const std::string& entryPoint)
			-> RayTracingPipelineBuilder<GroupState>
		{
			static_assert(GroupState == RayShaderGroup::eGeneral,
						  "Miss shaders can only be added to general shader groups.");
			addStageToGroup<vk::ShaderStageFlagBits::eMissKHR>(modulePath, entryPoint);
			return std::move(*this);
		}

		auto addStage(StageType<RayShaderStage::eCallable>, const std::string& modulePath, const std::string& entryPoint)
			-> RayTracingPipelineBuilder<GroupState>
		{
			static_assert(GroupState == RayShaderGroup::eGeneral,
						  "Callable shaders can only be added to general shader groups.");
			addStageToGroup<vk::ShaderStageFlagBits::eCallableKHR>(modulePath, entryPoint);
			return std::move(*this);
		}

		auto addStage(StageType<RayShaderStage::eClosestHit>, const std::string& modulePath, const std::string& entryPoint)
			-> RayTracingPipelineBuilder<GroupState>
		{
			static_assert(GroupState == RayShaderGroup::eTrianglesHitGroup
						  || GroupState == RayShaderGroup::eProceduralHitGroup,
						  "Closest hit shaders can only be added to triangle- or procedural hit groups.");
			addStageToGroup<vk::ShaderStageFlagBits::eClosestHitKHR>(modulePath, entryPoint);
			return std::move(*this);
		}

		auto addStage(StageType<RayShaderStage::eAnyHit>, const std::string& modulePath, const std::string& entryPoint)
			-> RayTracingPipelineBuilder<GroupState>
		{
			static_assert(GroupState == RayShaderGroup::eTrianglesHitGroup
						  || GroupState == RayShaderGroup::eProceduralHitGroup,
						  "Any hit shaders can only be added to triangle- or procedural hit groups.");
			addStageToGroup<vk::ShaderStageFlagBits::eAnyHitKHR>(modulePath, entryPoint);
			return std::move(*this);
		}

		auto addStage(StageType<RayShaderStage::eIntersection>, const std::string& modulePath, const std::string& entryPoint)
			-> RayTracingPipelineBuilder<GroupState>
		{
			static_assert(GroupState == RayShaderGroup::eProceduralHitGroup,
						  "Intersection shaders can only be added to procedural hit groups.");
			addStageToGroup<vk::ShaderStageFlagBits::eIntersectionKHR>(modulePath, entryPoint);
			return std::move(*this);
		}


		template<vk::ShaderStageFlagBits Stage>
		void addStageToGroup(const std::string& modulePath, const std::string& entryPoint);
		void finishGroup();

		ShaderGroup currentGroup;

		std::vector<vk::UniqueShaderModule> modules;
		std::vector<vk::PipelineShaderStageCreateInfo> stages;
		std::vector<vk::RayTracingShaderGroupCreateInfoKHR> groups;
	};


	/**
	 * @brief Build a ray tracing pipeline
	 */
	template<RayShaderGroup StartGroup = RayShaderGroup::eGeneral>
	auto inline buildRayTracingPipeline() -> RayTracingPipelineBuilder<StartGroup>
	{
		return RayTracingPipelineBuilder<StartGroup>();
	}
} // namespace trc::rt



// ------------------------ //
//		Implementations		//
// ------------------------ //

template<vk::ShaderStageFlagBits Stage>
struct StageArrayIndex : public std::integral_constant<size_t, 0> {};
template<>
struct StageArrayIndex<vk::ShaderStageFlagBits::eClosestHitKHR> : public std::integral_constant<size_t, 1> {};
template<>
struct StageArrayIndex<vk::ShaderStageFlagBits::eAnyHitKHR> : public std::integral_constant<size_t, 2> {};
template<>
struct StageArrayIndex<vk::ShaderStageFlagBits::eIntersectionKHR> : public std::integral_constant<size_t, 3> {};

template<vk::ShaderStageFlagBits Stage>
constexpr size_t StageArrayIndex_v = StageArrayIndex<Stage>::value;



template<trc::rt::RayShaderGroup GroupState>
template<trc::rt::RayShaderGroup OtherGroupState>
trc::rt::RayTracingPipelineBuilder<GroupState>::RayTracingPipelineBuilder(
	RayTracingPipelineBuilder<OtherGroupState>&& other) noexcept
	:
	currentGroup(std::move(other.currentGroup)),
	modules(std::move(other.modules)),
	stages(std::move(other.stages)),
	groups(std::move(other.groups))
{
}

template<trc::rt::RayShaderGroup GroupState>
template<trc::rt::RayShaderGroup Group>
auto trc::rt::RayTracingPipelineBuilder<GroupState>::addGroup() -> RayTracingPipelineBuilder<Group>
{
	finishGroup();
	return RayTracingPipelineBuilder<Group>(std::move(*this));
}

template<trc::rt::RayShaderGroup GroupState>
auto trc::rt::RayTracingPipelineBuilder<GroupState>::build(
	uint32_t maxRecursionDepth,
	vk::PipelineLayout layout) noexcept
{
	finishGroup();

	assert(stages.size() > 0);

	return vkb::getDevice()->createRayTracingPipelineKHRUnique(
        {}, // optional deferred operation
		{}, // cache
		vk::RayTracingPipelineCreateInfoKHR(
			vk::PipelineCreateFlags(),
			//static_cast<uint32_t>(stages.size()),
			stages, //.data(),
			//static_cast<uint32_t>(groups.size()),
			groups, //.data(),
			maxRecursionDepth, // max recursion depth
            nullptr, // pipeline library create info
            nullptr, // ray tracing pipeline interface create info
            nullptr, // dynamic state create info
			layout
		),
		nullptr, vkb::getDL()
	).value;
}

template<trc::rt::RayShaderGroup GroupState>
template<trc::rt::RayShaderStage Stage>
auto trc::rt::RayTracingPipelineBuilder<GroupState>::addStage(const std::string& modulePath, const std::string& entryPoint)
	-> trc::rt::RayTracingPipelineBuilder<GroupState>
{
	return addStage(StageType<Stage>(), modulePath, entryPoint);
}

template<trc::rt::RayShaderGroup GroupState>
template<vk::ShaderStageFlagBits Stage>
void trc::rt::RayTracingPipelineBuilder<GroupState>::addStageToGroup(
	const std::string& modulePath,
	const std::string& entryPoint)
{
	if (currentGroup.stageIndices[StageArrayIndex_v<Stage>] != VK_SHADER_UNUSED_KHR)
	{
		// Index is already occupied, tried to add the same shader slot twice
		throw std::runtime_error(
			"Tried to assign the same shader type twice to the same group."
			" Remember: raygen, miss, and callable shaders occupy the same"
			" slot. A group can only ever hold one of them.");
	}

	auto _module = vkb::createShaderModule(vkb::readFile(modulePath));
	currentGroup.stages.push_back({
        std::move(_module),
        vk::PipelineShaderStageCreateInfo{ {}, Stage, *_module, entryPoint.c_str() }
    });
	currentGroup.stageIndices[StageArrayIndex_v<Stage>] = currentGroup.stages.size() - 1;
}

template<trc::rt::RayShaderGroup GroupState>
void trc::rt::RayTracingPipelineBuilder<GroupState>::finishGroup()
{
	if (currentGroup.stages.size() == 0)
		return;

	uint32_t generalArg = currentGroup.stageIndices[0];
	uint32_t closestHitArg = currentGroup.stageIndices[1];
	uint32_t anyHitArg = currentGroup.stageIndices[2];
	uint32_t intersectArg = currentGroup.stageIndices[3];

	const uint32_t indexOffset = static_cast<uint32_t>(stages.size());
	groups.push_back(vk::RayTracingShaderGroupCreateInfoKHR{
		GroupState,
		generalArg == VK_SHADER_UNUSED_KHR ? VK_SHADER_UNUSED_KHR : generalArg + indexOffset,
		closestHitArg == VK_SHADER_UNUSED_KHR ? VK_SHADER_UNUSED_KHR : closestHitArg + indexOffset,
		anyHitArg == VK_SHADER_UNUSED_KHR ? VK_SHADER_UNUSED_KHR : anyHitArg + indexOffset,
		intersectArg == VK_SHADER_UNUSED_KHR ? VK_SHADER_UNUSED_KHR : intersectArg + indexOffset,
	});

	for (auto& stage : currentGroup.stages)
	{
		modules.push_back(std::move(stage.first));
		stages.push_back(stage.second);
	}

	currentGroup = {};
}
