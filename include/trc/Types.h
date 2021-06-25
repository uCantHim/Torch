#pragma once

#include <memory>

#include <vkb/VulkanInclude.h>

#define GLM_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#define TRC_FLIP_Y_PROJECTION

#include <nc/Types.h>
#include <nc/functional/Maybe.h>
#include <nc/data/TypesafeId.h>

/**
 * @brief Defines useful basic types
 *
 * These typedefs are used throughout Torch. You can do a
 *
 *     using namespace trc::basic_types;
 *
 * if you want to use these convenient type names with minimal namespace
 * pollution.
 */
namespace trc::basic_types
{
    using namespace nc::basic_types;
} // namespace trc::basic_types

namespace trc
{
    using namespace basic_types;
    using namespace nc;

    using nc::data::TypesafeID;
    using nc::functional::Maybe;
    using nc::functional::MaybeEmptyError;
} // namespace trc
