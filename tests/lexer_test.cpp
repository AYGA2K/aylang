#include "lexer/lexer.h"
#include "lexer/token.h"

#include <gtest/gtest.h>

#include <string>
#include <vector>

namespace {

struct Expected {
  TokenType type;
  std::string literal;
};

void expectTokens(const std::string &input,
                  const std::vector<Expected> &expected) {
  Lexer lexer{.input = input};
  std::vector<Token> got;
  for (;;) {
    Token tok = lexer.nextToken();
    if (tok.type == TokenType::EndOfFile) {
      break;
    }
    got.push_back(tok);
  }

  ASSERT_EQ(got.size(), expected.size());
  for (std::size_t i = 0; i < expected.size(); i++) {
    EXPECT_EQ(static_cast<int>(got[i].type), static_cast<int>(expected[i].type))
        << "type mismatch at token " << i << " (literal '" << got[i].literal
        << "')";
    EXPECT_EQ(got[i].literal, expected[i].literal)
        << "literal mismatch at token " << i;
  }
}

TEST(Lexer, SingleCharOperatorsAndPunctuation) {
  expectTokens("(){},;+-*/", {
                                 {TokenType::LParen, "("},
                                 {TokenType::RParen, ")"},
                                 {TokenType::LBrace, "{"},
                                 {TokenType::RBrace, "}"},
                                 {TokenType::Comma, ","},
                                 {TokenType::Semicolon, ";"},
                                 {TokenType::Plus, "+"},
                                 {TokenType::Minus, "-"},
                                 {TokenType::Star, "*"},
                                 {TokenType::Slash, "/"},
                             });
}

TEST(Lexer, TwoCharAndComparisonOperators) {
  expectTokens("= == ! != < <= > >= && ||",
               {
                   {TokenType::Assign, "="},
                   {TokenType::Equal, "=="},
                   {TokenType::Not, "!"},
                   {TokenType::NotEqual, "!="},
                   {TokenType::LessThan, "<"},
                   {TokenType::LessThanOrEqual, "<="},
                   {TokenType::GreaterThan, ">"},
                   {TokenType::GreaterThanOrEqual, ">="},
                   {TokenType::And, "&&"},
                   {TokenType::Or, "||"},
               });
}

TEST(Lexer, KeywordsVsIdentifiers) {
  expectTokens("if else while fn return foo _bar baz123",
               {
                   {TokenType::If, "if"},
                   {TokenType::Else, "else"},
                   {TokenType::While, "while"},
                   {TokenType::Function, "fn"},
                   {TokenType::Return, "return"},
                   {TokenType::Identifier, "foo"},
                   {TokenType::Identifier, "_bar"},
                   {TokenType::Identifier, "baz123"},
               });
}

TEST(Lexer, VariableDeclaration) {
  expectTokens("var x = 5;", {
                                 {TokenType::Var, "var"},
                                 {TokenType::Identifier, "x"},
                                 {TokenType::Assign, "="},
                                 {TokenType::Number, "5"},
                                 {TokenType::Semicolon, ";"},
                             });
}

TEST(Lexer, IntegerAndFloatNumbers) {
  expectTokens("0 42 3.14 10.0", {
                                     {TokenType::Number, "0"},
                                     {TokenType::Number, "42"},
                                     {TokenType::Number, "3.14"},
                                     {TokenType::Number, "10.0"},
                                 });
}

TEST(Lexer, TrailingDotIsNotConsumed) {
  // A '.' not followed by a digit is not part of the number.
  expectTokens("3.", {
                         {TokenType::Number, "3"},
                         {TokenType::Unknown, "."},
                     });
}

TEST(Lexer, WhitespaceIsSkipped) {
  expectTokens("  \t\n  x\t+\ny", {
                                      {TokenType::Identifier, "x"},
                                      {TokenType::Plus, "+"},
                                      {TokenType::Identifier, "y"},
                                  });
}

TEST(Lexer, LineCommentsAreIgnored) {
  expectTokens("a // this is a comment\nb", {
                                                {TokenType::Identifier, "a"},
                                                {TokenType::Identifier, "b"},
                                            });
}

TEST(Lexer, ASmallProgram) {
  expectTokens("fn add(x, y) { return x + y; }",
               {
                   {TokenType::Function, "fn"},
                   {TokenType::Identifier, "add"},
                   {TokenType::LParen, "("},
                   {TokenType::Identifier, "x"},
                   {TokenType::Comma, ","},
                   {TokenType::Identifier, "y"},
                   {TokenType::RParen, ")"},
                   {TokenType::LBrace, "{"},
                   {TokenType::Return, "return"},
                   {TokenType::Identifier, "x"},
                   {TokenType::Plus, "+"},
                   {TokenType::Identifier, "y"},
                   {TokenType::Semicolon, ";"},
                   {TokenType::RBrace, "}"},
               });
}

} // namespace
