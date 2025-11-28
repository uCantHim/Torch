#include <gtest/gtest.h>

#include <trc/material/shader/CodePrimitives.h>
#include <trc/material/shader/ShaderCodeBuilder.h>
#include <trc/material/shader/ShaderTypeChecker.h>

using namespace trc;
using namespace trc::shader;

class TestShaderCodeTypechecker : public testing::Test
{
protected:
    const std::vector<BasicType> kAllBasicTypes{
        i32{}, ui32{}, float{}, double{},
        vec2{}, vec3{}, vec4{},
        ivec2{}, ivec3{}, ivec4{},
        uvec2{}, uvec3{}, uvec4{},
        glm::dvec2{}, glm::dvec3{}, glm::dvec4{},
        mat3{}, mat4{}
    };

    const std::vector<BasicType> kAllVectorTypes{
        vec2{}, vec3{}, vec4{},
        ivec2{}, ivec3{}, ivec4{},
        uvec2{}, uvec3{}, uvec4{},
        glm::dvec2{}, glm::dvec3{}, glm::dvec4{},
        mat3{}, mat4{}
    };

    ShaderTypeChecker check;
    ShaderCodeBuilder builder;
};

TEST_F(TestShaderCodeTypechecker, LiteralType)
{
    std::vector<Constant> constants{
        i32{}, ui32{}, float{}, double{},
        vec2{}, vec3{}, vec4{},
        ivec2{}, ivec3{}, ivec4{},
        uvec2{}, uvec3{}, uvec4{},
        glm::dvec2{}, glm::dvec3{}, glm::dvec4{}
    };

    // Call operator() overload manually
    for (const auto& constant : constants) {
        ASSERT_EQ(check(code::Literal{ .value=constant }), constant.getType());
    }

    // Use the generic function that accepts the variant
    for (const auto& constant : constants) {
        ASSERT_EQ(check.inferType(builder.makeConstant(constant)), constant.getType());
    }
}

TEST_F(TestShaderCodeTypechecker, IdentifierType)
{
    ASSERT_FALSE(check(code::Identifier{ .name="" }));
    ASSERT_FALSE(check(code::Identifier{ .name="a" }));
    ASSERT_FALSE(check(code::Identifier{ .name="-awdm,2qe09awd awDaw.,,awa 2318qw|" }));
    ASSERT_FALSE(check(code::Identifier{ .name="foo" }));
    ASSERT_FALSE(check(code::Identifier{ .name="%" }));
    ASSERT_FALSE(check(code::Identifier{ .name="hello_world" }));

    ASSERT_FALSE(check.inferType(builder.makeExternalIdentifier("bar")));
}

TEST_F(TestShaderCodeTypechecker, FunctionCallType)
{
    for (const auto& type : kAllBasicTypes)
    {
        auto func = builder.makeOrGetFunction(type.to_string(), FunctionType{ {}, type });
        ASSERT_EQ(type, check(code::FunctionCall{ func, {} }));
        ASSERT_EQ(type, check.inferType(builder.makeCall(func, {})));
    }
}

TEST_F(TestShaderCodeTypechecker, FunctionCallTypeIsArgumentInvariant)
{
    for (const auto& type : kAllBasicTypes)
    {
        const FunctionType funcType{ { uvec2{}, float{} }, type };
        auto func = builder.makeOrGetFunction(type.to_string(), funcType);

        ASSERT_EQ(type, check(code::FunctionCall{ func, {} }));
        ASSERT_EQ(type, check.inferType(builder.makeCall(func, {})));
    }
}

TEST_F(TestShaderCodeTypechecker, SimpleMemberAccess)
{
    auto id = builder.makeExternalIdentifier("foo");
    ASSERT_FALSE(check.inferType(builder.makeMemberAccess(id, "bar")));

    for (const auto& t : kAllBasicTypes)
    {
        auto value = builder.makeConstant(t);
        ASSERT_FALSE(check.inferType(builder.makeMemberAccess(value, "xy")));
    }
}

TEST_F(TestShaderCodeTypechecker, SimpleArrayAccess)
{
    // An external identifier never has a type
    auto id = builder.makeExternalIdentifier("foo");
    ASSERT_FALSE(check.inferType(builder.makeArrayAccess(id, builder.makeConstant(3))));

    auto expr = builder.makeMul(builder.makeConstant(2), builder.makeConstant(13));
    auto makeArrayAccess = [&]<typename T>(T type) {
        return builder.makeArrayAccess(builder.makeConstant(type), expr);
    };

    ASSERT_EQ(BasicType{float{}}, check.inferType(makeArrayAccess(vec2{})));
    ASSERT_EQ(BasicType{float{}}, check.inferType(makeArrayAccess(vec3{})));
    ASSERT_EQ(BasicType{float{}}, check.inferType(makeArrayAccess(vec4{})));
    ASSERT_EQ(BasicType{float{}}, check.inferType(makeArrayAccess(mat3{})));
    ASSERT_EQ(BasicType{float{}}, check.inferType(makeArrayAccess(mat4{})));
    ASSERT_EQ(BasicType{int{}}, check.inferType(makeArrayAccess(ivec2{})));
    ASSERT_EQ(BasicType{int{}}, check.inferType(makeArrayAccess(ivec3{})));
    ASSERT_EQ(BasicType{int{}}, check.inferType(makeArrayAccess(ivec4{})));
    ASSERT_EQ(BasicType{uint{}}, check.inferType(makeArrayAccess(uvec2{})));
    ASSERT_EQ(BasicType{uint{}}, check.inferType(makeArrayAccess(uvec3{})));
    ASSERT_EQ(BasicType{uint{}}, check.inferType(makeArrayAccess(uvec4{})));
    ASSERT_EQ(BasicType{double{}}, check.inferType(makeArrayAccess(glm::dvec2{})));
    ASSERT_EQ(BasicType{double{}}, check.inferType(makeArrayAccess(glm::dvec3{})));
    ASSERT_EQ(BasicType{double{}}, check.inferType(makeArrayAccess(glm::dvec4{})));
    ASSERT_EQ(BasicType{double{}}, check.inferType(makeArrayAccess(glm::dmat3{})));
    ASSERT_EQ(BasicType{double{}}, check.inferType(makeArrayAccess(glm::dmat3{})));
    ASSERT_EQ(BasicType{double{}}, check.inferType(makeArrayAccess(glm::dmat4{})));
}

TEST_F(TestShaderCodeTypechecker, ComplexArrayAccess)
{
    ASSERT_FALSE(check.inferType(
        builder.makeArrayAccess(
            builder.makeExternalCall("foo", { builder.makeConstant(8u) }),
            builder.makeConstant(42)
        )
    ));
    ASSERT_FALSE(check.inferType(
        builder.makeArrayAccess(
            builder.makeConstant(2.71828),
            builder.makeExternalIdentifier("array_access_with_no_type")
        )
    ));

    auto makeFunctionCall = [&](BasicType type) {
        auto func = builder.makeOrGetFunction("bar_" + type.to_string(), FunctionType{ {}, type });
        auto call = builder.makeCall(func, {});
        return call;
    };
    auto index = builder.makeConstant(0);
    ASSERT_EQ(BasicType{float{}},
              check.inferType(builder.makeArrayAccess(makeFunctionCall(vec2{}), index)));
    ASSERT_EQ(BasicType{double{}},
              check.inferType(builder.makeArrayAccess(makeFunctionCall(glm::dvec4{}), index)));
    ASSERT_EQ(BasicType{double{}},
              check.inferType(builder.makeArrayAccess(makeFunctionCall(glm::dmat4{}), index)));
    ASSERT_EQ(BasicType{int{}},
              check.inferType(builder.makeArrayAccess(makeFunctionCall(ivec2{}), index)));
    ASSERT_EQ(BasicType{int{}},
              check.inferType(builder.makeArrayAccess(makeFunctionCall(ivec3{}), index)));
    ASSERT_EQ(BasicType{bool{}},
              check.inferType(builder.makeArrayAccess(makeFunctionCall(bvec4{}), index)));
}

TEST_F(TestShaderCodeTypechecker, UnaryOperatorTypeIsOperandType)
{
    for (const auto& type : kAllBasicTypes)
    {
        auto c = builder.makeConstant(Constant(type, {{ std::byte(0) }}));
        ASSERT_EQ(type, check(code::UnaryOperator{ "-", c }));
        ASSERT_EQ(type, check.inferType(builder.makeNot(c)));

        auto func = builder.makeOrGetFunction(type.to_string(), FunctionType{ {}, type });
        auto call = builder.makeCall(func, {});
        ASSERT_EQ(type, check(code::UnaryOperator{ ",", call }));

        auto access = builder.makeArrayAccess(c, builder.makeConstant(4));
        auto arrayAccessType = check.inferType(access);
        ASSERT_EQ(arrayAccessType, check(code::UnaryOperator{ "op", access }));
    }
}

TEST_F(TestShaderCodeTypechecker, BinaryOperatorSimpleTypes)
{
    // Create cross product of types
    std::vector<std::array<BasicType, 3>> config;
    for (const auto& lhs : kAllBasicTypes)
    {
        for (const auto& rhs : kAllBasicTypes)
        {
            config.push_back({ lhs, rhs, rhs });
            // Result type is lhs if (rhs < lhs)
            if (lhs.channels <= 4 && rhs.channels <= 4 && lhs.channels >= rhs.channels) {
                config.back()[2] = lhs;
            }
        }
    }

    for (const auto& [lhs, rhs, resultType] : config)
    {
        auto lhsVal = builder.makeConstant(Constant(lhs, {{ std::byte(0) }}));
        auto rhsVal = builder.makeConstant(Constant(rhs, {{ std::byte(0) }}));
        ASSERT_EQ(resultType, check(code::BinaryOperator{ "+", lhsVal, rhsVal }));
    }
}

TEST_F(TestShaderCodeTypechecker, BinaryOperatorBooleanOperators)
{
    // Create cross product of types
    std::vector<std::array<BasicType, 3>> config;
    for (const auto& lhs : kAllBasicTypes)
    {
        for (const auto& rhs : kAllBasicTypes)
        {
            config.push_back({ lhs, rhs, rhs });
            // Larger type is result type
            if (lhs.locations() == 1 && lhs.channels > rhs.channels) {
                config.back()[2] = lhs;
            }
        }
    }

    for (auto opName : { "<", ">", "<=", ">=", "==", "!=" })
    {
        for (const auto& [lhs, rhs, resultType] : config)
        {
            auto lhsVal = builder.makeConstant(Constant(lhs, {{ std::byte(0) }}));
            auto rhsVal = builder.makeConstant(Constant(rhs, {{ std::byte(0) }}));
            ASSERT_EQ(BasicType{ bool{} }, check(code::BinaryOperator{ opName, lhsVal, rhsVal }));
        }
    }
}
