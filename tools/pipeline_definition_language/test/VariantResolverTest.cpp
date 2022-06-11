#include <gtest/gtest.h>

#include <algorithm>
#include <string>
#include <vector>
#include <ranges>

#include "../src/ErrorReporter.h"
#include "../src/Scanner.h"
#include "../src/Parser.h"
#include "../src/FlagTable.h"
#include "../src/IdentifierTable.h"
#include "../src/VariantResolver.h"
#include "VariantFlagSetHash.h"

class TestErrorReporter : public ErrorReporter
{
public:
    void reportError(const Error&) override {}
};

class VariantResolverTest : public testing::Test
{
public:
    VariantResolverTest()
    {
        EnumTypeDef fooEnum(Token{ .type=TokenType::eEnum, .lexeme="foo_enum" });
        fooEnum.options.emplace(Token{ .type=TokenType::eIdentifier, .lexeme="foo_0" });
        fooEnum.options.emplace(Token{ .type=TokenType::eIdentifier, .lexeme="foo_1" });
        fooEnum.options.emplace(Token{ .type=TokenType::eIdentifier, .lexeme="foo_2" });
        EnumTypeDef barEnum(Token{ .type=TokenType::eEnum, .lexeme="bar_enum" });
        barEnum.options.emplace(Token{ .type=TokenType::eIdentifier, .lexeme="bar_0" });
        barEnum.options.emplace(Token{ .type=TokenType::eIdentifier, .lexeme="bar_1" });

        flagTable.registerFlagType(fooEnum);
        flagTable.registerFlagType(barEnum);
    }

protected:
    auto parse(std::string code) -> std::vector<Stmt>
    {
        Parser parser(Scanner(std::move(code), errorReporter).scanTokens(), errorReporter);
        return parser.parseTokens();
    }

    FlagTable flagTable;
    IdentifierTable idTable;

    VariantResolver resolver{ flagTable, idTable };

    TestErrorReporter errorReporter;
};

TEST_F(VariantResolverTest, SingleValue)
{
    std::string code = R"(
Thing foo: "hi"
)";
    auto ast = parse(code);

    auto vars = resolver.resolve(*std::get<FieldDefinition>(ast[0]).value);

    ASSERT_EQ(vars.size(), 1);
    ASSERT_EQ(vars[0].setFlags.size(), 0);
    ASSERT_TRUE(std::holds_alternative<LiteralValue>(vars[0].value));
    ASSERT_TRUE(std::holds_alternative<StringLiteral>(std::get<LiteralValue>(vars[0].value)));
}

TEST_F(VariantResolverTest, SimpleMatch)
{
    std::string code = R"(
Thing foo: match foo_enum
    foo_0 -> "first_var"
    foo_1 -> "second_var"
    foo_2 -> "third_var"
)";
    auto ast = parse(code);

    auto vars = resolver.resolve(*std::get<FieldDefinition>(ast[0]).value);

    ASSERT_EQ(vars.size(), 3);
    ASSERT_EQ(vars[0].setFlags.size(), 1);
    ASSERT_EQ(vars[1].setFlags.size(), 1);
    ASSERT_EQ(vars[2].setFlags.size(), 1);

    ASSERT_EQ(vars[0].setFlags[0].flagId,    flagTable.getRef("foo_enum", "foo_0").flagId);
    ASSERT_EQ(vars[0].setFlags[0].flagBitId, flagTable.getRef("foo_enum", "foo_0").flagBitId);
    ASSERT_EQ(vars[1].setFlags[0].flagId,    flagTable.getRef("foo_enum", "foo_1").flagId);
    ASSERT_EQ(vars[1].setFlags[0].flagBitId, flagTable.getRef("foo_enum", "foo_1").flagBitId);
    ASSERT_EQ(vars[2].setFlags[0].flagId,    flagTable.getRef("foo_enum", "foo_2").flagId);
    ASSERT_EQ(vars[2].setFlags[0].flagBitId, flagTable.getRef("foo_enum", "foo_2").flagBitId);

    ASSERT_TRUE(std::holds_alternative<LiteralValue>(vars[0].value));
    ASSERT_TRUE(std::holds_alternative<StringLiteral>(std::get<LiteralValue>(vars[0].value)));
    ASSERT_TRUE(std::holds_alternative<LiteralValue>(vars[1].value));
    ASSERT_TRUE(std::holds_alternative<StringLiteral>(std::get<LiteralValue>(vars[1].value)));
    ASSERT_TRUE(std::holds_alternative<LiteralValue>(vars[2].value));
    ASSERT_TRUE(std::holds_alternative<StringLiteral>(std::get<LiteralValue>(vars[2].value)));
}

TEST_F(VariantResolverTest, SimpleMatchOnField)
{
    std::string code = R"(
Object obj:
    first_field: match foo_enum
        foo_0 -> "first_var"
        foo_1 -> "second_var"
        foo_2 -> "third_var"
)";
    auto ast = parse(code);

    auto vars = resolver.resolve(*std::get<FieldDefinition>(ast[0]).value);

    ASSERT_EQ(vars.size(), 3);
    ASSERT_EQ(vars[0].setFlags.size(), 1);
    ASSERT_EQ(vars[1].setFlags.size(), 1);
    ASSERT_EQ(vars[2].setFlags.size(), 1);
}

TEST_F(VariantResolverTest, HomogenousMatchOnMultipleFields)
{
    std::string code = R"(
Object obj:
    first_field: match foo_enum
        foo_0 -> "first_var"
        foo_1 -> "second_var"
        foo_2 -> "third_var"
    second_field: match foo_enum
        foo_0 -> 1
        foo_1 -> 22
        foo_2 -> 333
)";
    auto ast = parse(code);

    auto vars = resolver.resolve(*std::get<FieldDefinition>(ast[0]).value);

    ASSERT_EQ(vars.size(), 3);
    ASSERT_EQ(vars[0].setFlags.size(), 1);
    ASSERT_EQ(vars[1].setFlags.size(), 1);
    ASSERT_EQ(vars[2].setFlags.size(), 1);
}

TEST_F(VariantResolverTest, HeterogenousMatchOnMultipleFields)
{
    std::string code = R"(
Object obj:
    first_field: match foo_enum
        foo_0 -> "first_var"
        foo_1 -> "second_var"
        foo_2 -> "third_var"
    second_field: match bar_enum
        bar_0 -> 1
        bar_1 -> 22
)";
    auto ast = parse(code);

    auto vars = resolver.resolve(*std::get<FieldDefinition>(ast[0]).value);

    ASSERT_EQ(vars.size(), 6);
    ASSERT_EQ(vars[0].setFlags.size(), 2);
    ASSERT_EQ(vars[1].setFlags.size(), 2);
    ASSERT_EQ(vars[2].setFlags.size(), 2);
    ASSERT_EQ(vars[3].setFlags.size(), 2);
    ASSERT_EQ(vars[4].setFlags.size(), 2);
    ASSERT_EQ(vars[5].setFlags.size(), 2);

    const auto fooFlag = flagTable.getRef("foo_enum", "foo_0");
    const auto barFlag = flagTable.getRef("bar_enum", "bar_0");
    ASSERT_EQ(vars[0].setFlags[0].flagId, fooFlag.flagId);
    ASSERT_EQ(vars[0].setFlags[1].flagId, barFlag.flagId);
    ASSERT_EQ(vars[0].setFlags[0].flagBitId, fooFlag.flagBitId);
    ASSERT_EQ(vars[0].setFlags[1].flagBitId, barFlag.flagBitId);
}

TEST_F(VariantResolverTest, MatchOnNestedObjects)
{
    std::string code = R"(
Object nested:
    my_field: match foo_enum
        foo_0 -> "first_var"
        foo_1 -> "second_var"
        foo_2 -> "third_var"

Object obj:
    first_field: nested
    second_field: match bar_enum
        bar_0 -> 1
        bar_1 -> 22
)";
    auto ast = parse(code);
    ASSERT_EQ(ast.size(), 2);

    idTable = IdentifierCollector(errorReporter).collect(ast);

    // Test nested object
    auto vars_nested = resolver.resolve(*std::get<FieldDefinition>(ast[0]).value);
    ASSERT_EQ(vars_nested.size(), 3);

    // Test composite object
    auto vars = resolver.resolve(*std::get<FieldDefinition>(ast[1]).value);
    ASSERT_EQ(vars.size(), 6);
    ASSERT_EQ(vars[0].setFlags.size(), 2);
    ASSERT_EQ(vars[1].setFlags.size(), 2);
    ASSERT_EQ(vars[2].setFlags.size(), 2);
    ASSERT_EQ(vars[3].setFlags.size(), 2);
    ASSERT_EQ(vars[4].setFlags.size(), 2);
    ASSERT_EQ(vars[5].setFlags.size(), 2);

    const auto fooFlag = flagTable.getRef("foo_enum", "foo_0");
    const auto barFlag = flagTable.getRef("bar_enum", "bar_0");
    ASSERT_EQ(vars[0].setFlags[0].flagId, fooFlag.flagId);
    ASSERT_EQ(vars[0].setFlags[1].flagId, barFlag.flagId);
    ASSERT_EQ(vars[1].setFlags[0].flagId, fooFlag.flagId);
    ASSERT_EQ(vars[1].setFlags[1].flagId, barFlag.flagId);
    ASSERT_EQ(vars[2].setFlags[0].flagId, fooFlag.flagId);
    ASSERT_EQ(vars[2].setFlags[1].flagId, barFlag.flagId);
    ASSERT_EQ(vars[3].setFlags[0].flagId, fooFlag.flagId);
    ASSERT_EQ(vars[3].setFlags[1].flagId, barFlag.flagId);
    ASSERT_EQ(vars[4].setFlags[0].flagId, fooFlag.flagId);
    ASSERT_EQ(vars[4].setFlags[1].flagId, barFlag.flagId);
    ASSERT_EQ(vars[5].setFlags[0].flagId, fooFlag.flagId);
    ASSERT_EQ(vars[5].setFlags[1].flagId, barFlag.flagId);

    ASSERT_EQ(vars[0].setFlags[0].flagBitId, fooFlag.flagBitId);
    ASSERT_EQ(vars[0].setFlags[1].flagBitId, barFlag.flagBitId);
}

TEST_F(VariantResolverTest, PartlySpecifiedNestedMatch)
{
    std::string code = R"(
Object obj: match foo_enum
    foo_0 -> match bar_enum
        bar_0 -> 11
        bar_1 -> 222
    foo_1 -> "string"
    foo_2 -> "hello"
)";
    auto ast = parse(code);
    ASSERT_EQ(ast.size(), 1);

    idTable = IdentifierCollector(errorReporter).collect(ast);

    // Test nested object
    // Test composite object
    auto vars = resolver.resolve(*std::get<FieldDefinition>(ast[0]).value);
    ASSERT_EQ(vars.size(), 6);
    ASSERT_EQ(vars[0].setFlags.size(), 2);
    ASSERT_EQ(vars[1].setFlags.size(), 2);
    ASSERT_EQ(vars[2].setFlags.size(), 2);
    ASSERT_EQ(vars[3].setFlags.size(), 2);
    ASSERT_EQ(vars[4].setFlags.size(), 2);
    ASSERT_EQ(vars[5].setFlags.size(), 2);
}

TEST_F(VariantResolverTest, DeeplyNestedRepeatedMatch)
{
    std::string code = R"(
Object obj: match foo_enum
    foo_0 -> "string"
    foo_1 -> match bar_enum
        bar_0 -> "11"
        bar_1 -> match foo_enum
            foo_0 -> "222"
            foo_1 -> "3333"
            foo_2 -> "44444"
    foo_2 -> match foo_enum
        foo_0 -> "hello 0"
        foo_1 -> "hello 1"
        foo_2 -> "hello 2"
)";
    auto ast = parse(code);
    ASSERT_EQ(ast.size(), 1);

    idTable = IdentifierCollector(errorReporter).collect(ast);

    auto vars = resolver.resolve(*std::get<FieldDefinition>(ast[0]).value);
    ASSERT_EQ(vars.size(), 6);
    ASSERT_EQ(vars[0].setFlags.size(), 2);
    ASSERT_EQ(vars[1].setFlags.size(), 2);
    ASSERT_EQ(vars[2].setFlags.size(), 2);
    ASSERT_EQ(vars[3].setFlags.size(), 2);
    ASSERT_EQ(vars[4].setFlags.size(), 2);
    ASSERT_EQ(vars[5].setFlags.size(), 2);

    std::unordered_map<VariantFlagSet, std::string> truthValues{
        { { flagTable.getRef("foo_enum", "foo_0"), flagTable.getRef("bar_enum", "bar_0") }, "string" },
        { { flagTable.getRef("foo_enum", "foo_0"), flagTable.getRef("bar_enum", "bar_1") }, "string" },
        { { flagTable.getRef("foo_enum", "foo_1"), flagTable.getRef("bar_enum", "bar_0") }, "11" },
        { { flagTable.getRef("foo_enum", "foo_1"), flagTable.getRef("bar_enum", "bar_1") }, "3333" },
        { { flagTable.getRef("foo_enum", "foo_2"), flagTable.getRef("bar_enum", "bar_0") }, "hello 2" },
        { { flagTable.getRef("foo_enum", "foo_2"), flagTable.getRef("bar_enum", "bar_1") }, "hello 2" },
    };
    for (const auto& var : vars)
    {
        auto& value = var.value;
        auto& truth = truthValues.at(var.setFlags);

        ASSERT_TRUE(std::holds_alternative<LiteralValue>(value));
        ASSERT_TRUE(std::holds_alternative<StringLiteral>(std::get<LiteralValue>(value)));
        ASSERT_EQ(std::get<StringLiteral>(std::get<LiteralValue>(value)).value, truth);
    }
}
