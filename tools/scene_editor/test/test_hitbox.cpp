#include <gtest/gtest.h>

#include <glm/glm.hpp>
#include <glm/gtx/norm.hpp>
using vec3 = glm::vec3;

#include "Hitbox.cpp"

int main()
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

    ASSERT_TRUE(isInCapsule(in, capsule));
    ASSERT_TRUE(isInCapsule(in2, capsule));
    ASSERT_TRUE(isInCapsule(in3, capsule));
    ASSERT_TRUE(isInCapsule(vec3{ 0.0f, 1.9f, 0.0f }, capsule));
    ASSERT_TRUE(!isInCapsule(out, capsule));
    ASSERT_TRUE(!isInCapsule(vec3{ 0.0f, 3.0f, 0.0f }, capsule));
    ASSERT_TRUE(!isInCapsule(vec3{ 0.0f, 4.0f, 0.0f }, capsule));
    ASSERT_TRUE(!isInCapsule(out2, capsule));
    ASSERT_TRUE(!isInCapsule(out3, capsule));
    ASSERT_TRUE(!isInCapsule(out4, capsule));
    ASSERT_TRUE(!isInCapsule(out5, capsule));

    return 0;
}
