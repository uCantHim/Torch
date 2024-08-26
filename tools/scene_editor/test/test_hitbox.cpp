#include <gtest/gtest.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/norm.hpp>
using vec3 = glm::vec3;

#include "object/Hitbox.h"

TEST(HitboxTest, Capsule)
{
    Capsule capsule(2.0f, 0.5f);

    vec3 in{ 0.25f, 0.5f, -0.25f };
    vec3 in2{ 0.0f, 0.5f, 0.0f };
    vec3 in3{ 0.0f, 0.001f, 0.0f };
    vec3 out{ 0.1f, 2.1f, 0.0f };  // True at y = 3.32288
    vec3 out2{ 0.0f, -0.1f, 0.0f };
    vec3 out3{ -1.0f, 0.4f, 0.0f };
    vec3 out4{ 0.5f, 0.5f, 0.5f };
    vec3 out5{ 0.25f, 0.1f, 0.25f };

    ASSERT_TRUE(isInside(in, capsule));
    ASSERT_TRUE(isInside(in2, capsule));
    ASSERT_TRUE(isInside(in3, capsule));
    ASSERT_TRUE(isInside(vec3{ 0.0f, 1.9f, 0.0f }, capsule));
    ASSERT_TRUE(!isInside(out, capsule));
    ASSERT_TRUE(!isInside(vec3{ 0.0f, 3.0f, 0.0f }, capsule));
    ASSERT_TRUE(!isInside(vec3{ 0.0f, 4.0f, 0.0f }, capsule));
    ASSERT_TRUE(!isInside(out2, capsule));
    ASSERT_TRUE(!isInside(out3, capsule));
    ASSERT_TRUE(!isInside(out4, capsule));
    ASSERT_TRUE(!isInside(out5, capsule));
}
