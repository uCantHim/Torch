#include <gtest/gtest.h>

#include "../src/ErrorReporter.h"
#include "../src/Scanner.h"

class TestErrorReporter : public ErrorReporter
{
public:
    void reportError(const Error&) override {}
};

// NOLINTNEXTLINE
TEST(ScannerTest, ScanEmptyString)
{
    TestErrorReporter er;
    Scanner scanner("", er);

    auto tokens = scanner.scanTokens();
    ASSERT_FALSE(er.hadError());
    ASSERT_EQ(tokens.size(), 3);
    ASSERT_EQ(tokens[0].type, TokenType::eNewline);
    ASSERT_EQ(tokens[1].type, TokenType::eIndent);
    ASSERT_EQ(tokens[2].type, TokenType::eEof);
}

// NOLINTNEXTLINE
TEST(ScannerTest, ScanSingleCharacter)
{
    TestErrorReporter er;
    Scanner scanner("a", er);

    auto tokens = scanner.scanTokens();
    ASSERT_FALSE(er.hadError());
    ASSERT_EQ(tokens.size(), 4);
    ASSERT_EQ(tokens[2].type, TokenType::eIdentifier);
    ASSERT_EQ(tokens[2].lexeme, "a");
    ASSERT_EQ(tokens[3].type, TokenType::eEof);
}

// NOLINTNEXTLINE
TEST(ScannerTest, ScanSingleNewline)
{
    TestErrorReporter er;
    Scanner scanner(R"(
)", er);

    // The scanner has to remove redundant newlines and indents
    auto tokens = scanner.scanTokens();
    ASSERT_FALSE(er.hadError());
    ASSERT_EQ(tokens.size(), 3);
    ASSERT_EQ(tokens[0].type, TokenType::eNewline);
    ASSERT_EQ(tokens[1].type, TokenType::eIndent);
    ASSERT_EQ(tokens[2].type, TokenType::eEof);
}

// NOLINTNEXTLINE
TEST(ScannerTest, ScanInvalidCharacter)
{
    TestErrorReporter er;
    Scanner scanner("^", er);

    auto tokens = scanner.scanTokens();
    ASSERT_TRUE(er.hadError());
    ASSERT_EQ(tokens.size(), 3);
}

// NOLINTNEXTLINE
TEST(ScannerTest, ScanMultilineString)
{
    TestErrorReporter er;
    Scanner scanner(R"("Hello
 World!")", er);

    scanner.scanTokens();
    ASSERT_TRUE(er.hadError());
}
