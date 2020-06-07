#pragma once

#include <vulkan/vulkan.hpp>

#include "SelfManagedObject.h"

// Will probably not be used but I can define a consistent ID type this way
struct SubPass
{
    using ID = SelfManagedObject<SubPass>::ID;
};

/**
 * @brief A renderpass
 *
 * Contains subpasses.
 */
class RenderPass : public SelfManagedObject<RenderPass>
{
public:
    auto getSubPasses() -> const std::vector<SubPass::ID>&;

private:
    vk::UniqueRenderPass renderPass;

    // Just a list of contiguous numbers. Could be a simple int but I like
    // the consistency of having range-based for loops everywhere.
    std::vector<SubPass::ID> subpasses;
};
