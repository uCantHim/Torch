#include <stdexcept>

#include <gtest/gtest.h>

#include "Parser.h"
#include "Scanner.h"
#include "TestingErrorReporter.h"

auto parseAst(std::string str)
{
    TestingErrorReporter errorReporter;

    Scanner scanner(std::move(str), errorReporter);
    auto tokens = scanner.scanTokens();
    if (errorReporter.hadError()) throw std::runtime_error("Scanning failed for test string");

    Parser parser(std::move(tokens), errorReporter);
    auto ast = parser.parseTokens();
    if (errorReporter.hadError()) throw std::runtime_error("Parsing failed for test string");

    return ast;
}

TEST(EnumTest, EnumOptionsParsedInOrder)
{
    constexpr const char* code0 = R"(
enum MyTestEnum:
    foo,
    bar,
    baz,
    thing
)";
    constexpr const char* code1 = R"(
enum MyTestEnum:
    baz,
    thing,
    bar,
    foo
)";

    {
        auto ast = parseAst(code0);
        ASSERT_EQ(ast.size(), 1);
        ASSERT_TRUE(std::holds_alternative<TypeDef>(ast.front()));

        TypeDef& stmt = std::get<TypeDef>(ast.front());
        ASSERT_TRUE(std::holds_alternative<EnumTypeDef>(stmt));

        EnumTypeDef& enumType = std::get<EnumTypeDef>(stmt);
        ASSERT_EQ(enumType.options.at(0).value, "foo");
        ASSERT_EQ(enumType.options.at(1).value, "bar");
        ASSERT_EQ(enumType.options.at(2).value, "baz");
        ASSERT_EQ(enumType.options.at(3).value, "thing");
    }
    {
        auto ast = parseAst(code1);
        ASSERT_EQ(ast.size(), 1);
        ASSERT_TRUE(std::holds_alternative<TypeDef>(ast.front()));

        TypeDef& stmt = std::get<TypeDef>(ast.front());
        ASSERT_TRUE(std::holds_alternative<EnumTypeDef>(stmt));

        EnumTypeDef& enumType = std::get<EnumTypeDef>(stmt);
        ASSERT_EQ(enumType.options.at(0).value, "baz");
        ASSERT_EQ(enumType.options.at(1).value, "thing");
        ASSERT_EQ(enumType.options.at(2).value, "bar");
        ASSERT_EQ(enumType.options.at(3).value, "foo");
    }
}
