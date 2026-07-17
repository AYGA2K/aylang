#include "lexer/lexer.h"
#include "lexer/token.h"
#include "parser/expression.h"
#include "parser/parser.h"

#include <gtest/gtest.h>

#include <string>
#include <vector>

namespace {

std::vector<Token> tokenize(const std::string &input) {
  Lexer lexer{.input = input};
  return lexer.tokenize();
}

TEST(Parser, ParseIdentifier) {
  std::vector<Token> tokens = tokenize("foobar;");
  Parser parser{tokens};

  Expression expression = parser.parseIdentifier();

  EXPECT_EQ(static_cast<int>(expression.kind),
            static_cast<int>(ExpressionKind::IDENTIFIER));
  EXPECT_EQ(expression.stringValue, "foobar");
  EXPECT_TRUE(parser.errors.empty());
}

TEST(Parser, ParseNumberInteger) {
  std::vector<Token> tokens = tokenize("42;");
  Parser parser{tokens};

  Expression expression = parser.parseNumber();

  EXPECT_EQ(static_cast<int>(expression.kind),
            static_cast<int>(ExpressionKind::LITERAL_NUMBER));
  EXPECT_DOUBLE_EQ(expression.numValue, 42.0);
  EXPECT_TRUE(parser.errors.empty());
}

TEST(Parser, ParseNumberFloat) {
  std::vector<Token> tokens = tokenize("3.14;");
  Parser parser{tokens};

  Expression expression = parser.parseNumber();

  EXPECT_EQ(static_cast<int>(expression.kind),
            static_cast<int>(ExpressionKind::LITERAL_NUMBER));
  EXPECT_DOUBLE_EQ(expression.numValue, 3.14);
  EXPECT_TRUE(parser.errors.empty());
}

TEST(Parser, ParsePrefixBang) {
  std::vector<Token> tokens = tokenize("!5;");
  Parser parser{tokens};

  Expression expression = parser.parsePrefix();

  EXPECT_EQ(static_cast<int>(expression.kind),
            static_cast<int>(ExpressionKind::PREFIX));
  EXPECT_EQ(static_cast<int>(expression.unaryOperator),
            static_cast<int>(UnaryOperator::NOT));
  ASSERT_GE(expression.operandExprIndex, 0);
  Expression operand = parser.parserResult.expressions[expression.operandExprIndex];
  EXPECT_EQ(static_cast<int>(operand.kind),
            static_cast<int>(ExpressionKind::LITERAL_NUMBER));
  EXPECT_DOUBLE_EQ(operand.numValue, 5.0);
  EXPECT_TRUE(parser.errors.empty());
}

TEST(Parser, ParsePrefixMinus) {
  std::vector<Token> tokens = tokenize("-15;");
  Parser parser{tokens};

  Expression expression = parser.parsePrefix();

  EXPECT_EQ(static_cast<int>(expression.kind),
            static_cast<int>(ExpressionKind::PREFIX));
  EXPECT_EQ(static_cast<int>(expression.unaryOperator),
            static_cast<int>(UnaryOperator::NEGATE));
  ASSERT_GE(expression.operandExprIndex, 0);
  Expression operand = parser.parserResult.expressions[expression.operandExprIndex];
  EXPECT_EQ(static_cast<int>(operand.kind),
            static_cast<int>(ExpressionKind::LITERAL_NUMBER));
  EXPECT_DOUBLE_EQ(operand.numValue, 15.0);
  EXPECT_TRUE(parser.errors.empty());
}

TEST(Parser, ParsePrefixBangIdentifier) {
  std::vector<Token> tokens = tokenize("!foobar;");
  Parser parser{tokens};

  Expression expression = parser.parsePrefix();

  EXPECT_EQ(static_cast<int>(expression.kind),
            static_cast<int>(ExpressionKind::PREFIX));
  EXPECT_EQ(static_cast<int>(expression.unaryOperator),
            static_cast<int>(UnaryOperator::NOT));
  ASSERT_GE(expression.operandExprIndex, 0);
  Expression operand = parser.parserResult.expressions[expression.operandExprIndex];
  EXPECT_EQ(static_cast<int>(operand.kind),
            static_cast<int>(ExpressionKind::IDENTIFIER));
  EXPECT_EQ(operand.stringValue, "foobar");
  EXPECT_TRUE(parser.errors.empty());
}

TEST(Parser, ParsePrefixNested) {
  std::vector<Token> tokens = tokenize("!!5;");
  Parser parser{tokens};

  Expression expression = parser.parsePrefix();

  EXPECT_EQ(static_cast<int>(expression.kind),
            static_cast<int>(ExpressionKind::PREFIX));
  EXPECT_EQ(static_cast<int>(expression.unaryOperator),
            static_cast<int>(UnaryOperator::NOT));
  ASSERT_GE(expression.operandExprIndex, 0);
  Expression inner = parser.parserResult.expressions[expression.operandExprIndex];
  EXPECT_EQ(static_cast<int>(inner.kind),
            static_cast<int>(ExpressionKind::PREFIX));
  EXPECT_EQ(static_cast<int>(inner.unaryOperator),
            static_cast<int>(UnaryOperator::NOT));
  ASSERT_GE(inner.operandExprIndex, 0);
  Expression operand = parser.parserResult.expressions[inner.operandExprIndex];
  EXPECT_EQ(static_cast<int>(operand.kind),
            static_cast<int>(ExpressionKind::LITERAL_NUMBER));
  EXPECT_DOUBLE_EQ(operand.numValue, 5.0);
  EXPECT_TRUE(parser.errors.empty());
}

TEST(Parser, IdentifierExpressionStatement) {
  std::vector<Token> tokens = tokenize("foobar;");
  Parser parser{tokens};

  Statement statement = parser.parseExpressionStatment();

  EXPECT_EQ(static_cast<int>(statement.kind),
            static_cast<int>(StatementKind::EXPRESSION));
  EXPECT_EQ(static_cast<int>(statement.expression.kind),
            static_cast<int>(ExpressionKind::IDENTIFIER));
  EXPECT_EQ(statement.expression.stringValue, "foobar");
  EXPECT_TRUE(parser.errors.empty());
}

TEST(Parser, NumberExpressionStatement) {
  std::vector<Token> tokens = tokenize("5;");
  Parser parser{tokens};

  Statement statement = parser.parseExpressionStatment();

  EXPECT_EQ(static_cast<int>(statement.kind),
            static_cast<int>(StatementKind::EXPRESSION));
  EXPECT_EQ(static_cast<int>(statement.expression.kind),
            static_cast<int>(ExpressionKind::LITERAL_NUMBER));
  EXPECT_DOUBLE_EQ(statement.expression.numValue, 5.0);
  EXPECT_TRUE(parser.errors.empty());
}

} // namespace
