#pragma once

#include <memory>

#define GLM_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "trc_util/functional/Maybe.h"
#include "trc_util/data/TypesafeId.h"

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
    using int8 = int8_t;
    using int16 = int16_t;
    using int32 = int32_t;
    using int64 = int64_t;

    using i8 = int8;
    using i16 = int16;
    using i32 = int32;
    using i64 = int64;

    using uint8 = uint8_t;
    using uint16 = uint16_t;
    using uint32 = uint32_t;
    using uint64 = uint64_t;

    using ui8 = uint8;
    using ui16 = uint16;
    using ui32 = uint32;
    using ui64 = uint64;

    using bool32 = ui32;


    // ----------------------- //
    // Vector and matrix types //
    // ----------------------- //

    using vec2 = glm::vec2;
    using vec3 = glm::vec3;
    using vec4 = glm::vec4;
    using ivec2 = glm::ivec2;
    using ivec3 = glm::ivec3;
    using ivec4 = glm::ivec4;
    using uvec2 = glm::uvec2;
    using uvec3 = glm::uvec3;
    using uvec4 = glm::uvec4;

    using mat2 = glm::mat2;
    using mat3 = glm::mat3;
    using mat4 = glm::mat4;

    using quat = glm::quat;


    // --------------------- //
    // Smart pointer aliases //
    // --------------------- //

    template<typename T, typename D = std::default_delete<T>>
    using u_ptr = std::unique_ptr<T, D>;
    template<typename T>
    using s_ptr = std::shared_ptr<T>;
    template<typename T>
    using w_ptr = std::weak_ptr<T>;


    // ----------------------- //
    // Frequently used helpers //
    // ----------------------- //

    using data::TypesafeID;
    using functional::Maybe;
    using functional::MaybeEmptyError;
} // namespace trc::basic_types

namespace trc
{
    using namespace basic_types;
} // namespace trc

/**
 * Expose Torch's interface with vk_base completely integrated into the trc
 * namespace to avoid confusion.
 */
#ifdef TRC_USE_VKB_NAMESPACE
namespace vkb {}

namespace trc
{
    using namespace vkb;
} // namespace trc
#endif
