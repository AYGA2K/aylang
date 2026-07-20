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

Expression at(Parser &parser, int index) {
  return parser.parserResult.expressions[index];
}

TEST(Parser, ParseIdentifier) {
  std::vector<Token> tokens = tokenize("foobar;");
  Parser parser{tokens};

  int index = parser.parseIdentifier();
  Expression expression = parser.parserResult.expressions[index];

  EXPECT_EQ(static_cast<int>(expression.kind),
            static_cast<int>(ExpressionKind::IDENTIFIER));
  EXPECT_EQ(expression.stringValue, "foobar");
  EXPECT_TRUE(parser.errors.empty());
}

TEST(Parser, ParseNumberInteger) {
  std::vector<Token> tokens = tokenize("42;");
  Parser parser{tokens};

  int index = parser.parseNumber();
  Expression expression = parser.parserResult.expressions[index];

  EXPECT_EQ(static_cast<int>(expression.kind),
            static_cast<int>(ExpressionKind::LITERAL_NUMBER));
  EXPECT_DOUBLE_EQ(expression.numValue, 42.0);
  EXPECT_TRUE(parser.errors.empty());
}

TEST(Parser, ParseNumberFloat) {
  std::vector<Token> tokens = tokenize("3.14;");
  Parser parser{tokens};

  int index = parser.parseNumber();
  Expression expression = parser.parserResult.expressions[index];

  EXPECT_EQ(static_cast<int>(expression.kind),
            static_cast<int>(ExpressionKind::LITERAL_NUMBER));
  EXPECT_DOUBLE_EQ(expression.numValue, 3.14);
  EXPECT_TRUE(parser.errors.empty());
}

TEST(Parser, ParseBooleanTrue) {
  std::vector<Token> tokens = tokenize("true;");
  Parser parser{tokens};

  int index = parser.parseBoolean();
  Expression expression = parser.parserResult.expressions[index];

  EXPECT_EQ(static_cast<int>(expression.kind),
            static_cast<int>(ExpressionKind::LITERAL_BOOL));
  EXPECT_TRUE(expression.boolValue);
  EXPECT_TRUE(parser.errors.empty());
}

TEST(Parser, ParseBooleanFalse) {
  std::vector<Token> tokens = tokenize("false;");
  Parser parser{tokens};

  int index = parser.parseBoolean();
  Expression expression = parser.parserResult.expressions[index];

  EXPECT_EQ(static_cast<int>(expression.kind),
            static_cast<int>(ExpressionKind::LITERAL_BOOL));
  EXPECT_FALSE(expression.boolValue);
  EXPECT_TRUE(parser.errors.empty());
}

TEST(Parser, ParseUnaryBang) {
  std::vector<Token> tokens = tokenize("!5;");
  Parser parser{tokens};

  int index = parser.parseUnary();
  Expression expression = parser.parserResult.expressions[index];

  EXPECT_EQ(static_cast<int>(expression.kind),
            static_cast<int>(ExpressionKind::UNARY));
  EXPECT_EQ(static_cast<int>(expression.unaryOperator),
            static_cast<int>(UnaryOperator::NOT));
  ASSERT_GE(expression.operandExprIndex, 0);
  Expression operand =
      parser.parserResult.expressions[expression.operandExprIndex];
  EXPECT_EQ(static_cast<int>(operand.kind),
            static_cast<int>(ExpressionKind::LITERAL_NUMBER));
  EXPECT_DOUBLE_EQ(operand.numValue, 5.0);
  EXPECT_TRUE(parser.errors.empty());
}

TEST(Parser, ParseUnaryMinus) {
  std::vector<Token> tokens = tokenize("-15;");
  Parser parser{tokens};

  int index = parser.parseUnary();
  Expression expression = parser.parserResult.expressions[index];

  EXPECT_EQ(static_cast<int>(expression.kind),
            static_cast<int>(ExpressionKind::UNARY));
  EXPECT_EQ(static_cast<int>(expression.unaryOperator),
            static_cast<int>(UnaryOperator::NEGATE));
  ASSERT_GE(expression.operandExprIndex, 0);
  Expression operand =
      parser.parserResult.expressions[expression.operandExprIndex];
  EXPECT_EQ(static_cast<int>(operand.kind),
            static_cast<int>(ExpressionKind::LITERAL_NUMBER));
  EXPECT_DOUBLE_EQ(operand.numValue, 15.0);
  EXPECT_TRUE(parser.errors.empty());
}

TEST(Parser, ParseUnaryBangIdentifier) {
  std::vector<Token> tokens = tokenize("!foobar;");
  Parser parser{tokens};

  int index = parser.parseUnary();
  Expression expression = parser.parserResult.expressions[index];

  EXPECT_EQ(static_cast<int>(expression.kind),
            static_cast<int>(ExpressionKind::UNARY));
  EXPECT_EQ(static_cast<int>(expression.unaryOperator),
            static_cast<int>(UnaryOperator::NOT));
  ASSERT_GE(expression.operandExprIndex, 0);
  Expression operand =
      parser.parserResult.expressions[expression.operandExprIndex];
  EXPECT_EQ(static_cast<int>(operand.kind),
            static_cast<int>(ExpressionKind::IDENTIFIER));
  EXPECT_EQ(operand.stringValue, "foobar");
  EXPECT_TRUE(parser.errors.empty());
}

TEST(Parser, ParseUnaryNested) {
  std::vector<Token> tokens = tokenize("!!5;");
  Parser parser{tokens};

  int index = parser.parseUnary();
  Expression expression = parser.parserResult.expressions[index];

  EXPECT_EQ(static_cast<int>(expression.kind),
            static_cast<int>(ExpressionKind::UNARY));
  EXPECT_EQ(static_cast<int>(expression.unaryOperator),
            static_cast<int>(UnaryOperator::NOT));
  ASSERT_GE(expression.operandExprIndex, 0);
  Expression inner =
      parser.parserResult.expressions[expression.operandExprIndex];
  EXPECT_EQ(static_cast<int>(inner.kind),
            static_cast<int>(ExpressionKind::UNARY));
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

  parser.parseExpressionStatment();

  ASSERT_FALSE(parser.parserResult.statements.empty());
  Statement statement = parser.parserResult.statements[0];
  EXPECT_EQ(static_cast<int>(statement.kind),
            static_cast<int>(StatementKind::EXPRESSION));
  ASSERT_GE(statement.expressionIndex, 0);
  Expression expression =
      parser.parserResult.expressions[statement.expressionIndex];
  EXPECT_EQ(static_cast<int>(expression.kind),
            static_cast<int>(ExpressionKind::IDENTIFIER));
  EXPECT_EQ(expression.stringValue, "foobar");
  EXPECT_TRUE(parser.errors.empty());
}

TEST(Parser, NumberExpressionStatement) {
  std::vector<Token> tokens = tokenize("5;");
  Parser parser{tokens};

  parser.parseExpressionStatment();

  ASSERT_FALSE(parser.parserResult.statements.empty());
  Statement statement = parser.parserResult.statements[0];
  EXPECT_EQ(static_cast<int>(statement.kind),
            static_cast<int>(StatementKind::EXPRESSION));
  ASSERT_GE(statement.expressionIndex, 0);
  Expression expression =
      parser.parserResult.expressions[statement.expressionIndex];
  EXPECT_EQ(static_cast<int>(expression.kind),
            static_cast<int>(ExpressionKind::LITERAL_NUMBER));
  EXPECT_DOUBLE_EQ(expression.numValue, 5.0);
  EXPECT_TRUE(parser.errors.empty());
}

TEST(Parser, ParseUnaryBangBoolean) {
  std::vector<Token> tokens = tokenize("!true;");
  Parser parser{tokens};

  int index = parser.parseUnary();
  Expression expression = parser.parserResult.expressions[index];

  EXPECT_EQ(static_cast<int>(expression.kind),
            static_cast<int>(ExpressionKind::UNARY));
  EXPECT_EQ(static_cast<int>(expression.unaryOperator),
            static_cast<int>(UnaryOperator::NOT));
  ASSERT_GE(expression.operandExprIndex, 0);
  Expression operand =
      parser.parserResult.expressions[expression.operandExprIndex];
  EXPECT_EQ(static_cast<int>(operand.kind),
            static_cast<int>(ExpressionKind::LITERAL_BOOL));
  EXPECT_TRUE(operand.boolValue);
  EXPECT_TRUE(parser.errors.empty());
}

TEST(Parser, BooleanExpressionStatementTrue) {
  std::vector<Token> tokens = tokenize("true;");
  Parser parser{tokens};

  parser.parseExpressionStatment();

  ASSERT_FALSE(parser.parserResult.statements.empty());
  Statement statement = parser.parserResult.statements[0];
  EXPECT_EQ(static_cast<int>(statement.kind),
            static_cast<int>(StatementKind::EXPRESSION));
  ASSERT_GE(statement.expressionIndex, 0);
  Expression expression =
      parser.parserResult.expressions[statement.expressionIndex];
  EXPECT_EQ(static_cast<int>(expression.kind),
            static_cast<int>(ExpressionKind::LITERAL_BOOL));
  EXPECT_TRUE(expression.boolValue);
  EXPECT_TRUE(parser.errors.empty());
}

TEST(Parser, BooleanExpressionStatementFalse) {
  std::vector<Token> tokens = tokenize("false;");
  Parser parser{tokens};

  parser.parseExpressionStatment();

  ASSERT_FALSE(parser.parserResult.statements.empty());
  Statement statement = parser.parserResult.statements[0];
  EXPECT_EQ(static_cast<int>(statement.kind),
            static_cast<int>(StatementKind::EXPRESSION));
  ASSERT_GE(statement.expressionIndex, 0);
  Expression expression =
      parser.parserResult.expressions[statement.expressionIndex];
  EXPECT_EQ(static_cast<int>(expression.kind),
            static_cast<int>(ExpressionKind::LITERAL_BOOL));
  EXPECT_FALSE(expression.boolValue);
  EXPECT_TRUE(parser.errors.empty());
}

TEST(Parser, ParseBinaryAdd) {
  std::vector<Token> tokens = tokenize("1 + 2;");
  Parser parser{tokens};

  parser.parseExpressionStatment();

  ASSERT_FALSE(parser.parserResult.statements.empty());
  Expression expression =
      at(parser, parser.parserResult.statements[0].expressionIndex);
  EXPECT_EQ(static_cast<int>(expression.kind),
            static_cast<int>(ExpressionKind::BINARY));
  EXPECT_EQ(static_cast<int>(expression.binaryOperator),
            static_cast<int>(BinaryOperator::ADD));

  Expression left = at(parser, expression.leftExprIndex);
  Expression right = at(parser, expression.rightExprIndex);
  EXPECT_DOUBLE_EQ(left.numValue, 1.0);
  EXPECT_DOUBLE_EQ(right.numValue, 2.0);
  EXPECT_TRUE(parser.errors.empty());
}

TEST(Parser, ParseBinaryProductBindsTighterThanSum) {
  std::vector<Token> tokens = tokenize("1 + 2 * 3;");
  Parser parser{tokens};

  parser.parseExpressionStatment();

  ASSERT_FALSE(parser.parserResult.statements.empty());
  // Expect 1 + (2 * 3): root is ADD, right child is MULTIPLY.
  Expression root =
      at(parser, parser.parserResult.statements[0].expressionIndex);
  EXPECT_EQ(static_cast<int>(root.kind),
            static_cast<int>(ExpressionKind::BINARY));
  EXPECT_EQ(static_cast<int>(root.binaryOperator),
            static_cast<int>(BinaryOperator::ADD));

  Expression left = at(parser, root.leftExprIndex);
  EXPECT_DOUBLE_EQ(left.numValue, 1.0);

  Expression right = at(parser, root.rightExprIndex);
  EXPECT_EQ(static_cast<int>(right.kind),
            static_cast<int>(ExpressionKind::BINARY));
  EXPECT_EQ(static_cast<int>(right.binaryOperator),
            static_cast<int>(BinaryOperator::MULTIPLY));
  EXPECT_DOUBLE_EQ(at(parser, right.leftExprIndex).numValue, 2.0);
  EXPECT_DOUBLE_EQ(at(parser, right.rightExprIndex).numValue, 3.0);
  EXPECT_TRUE(parser.errors.empty());
}

TEST(Parser, ParseBinaryLeftAssociative) {
  std::vector<Token> tokens = tokenize("1 - 2 - 3;");
  Parser parser{tokens};

  parser.parseExpressionStatment();

  ASSERT_FALSE(parser.parserResult.statements.empty());
  // Expect (1 - 2) - 3: root is SUBTRACT, left child is SUBTRACT.
  Expression root =
      at(parser, parser.parserResult.statements[0].expressionIndex);
  EXPECT_EQ(static_cast<int>(root.kind),
            static_cast<int>(ExpressionKind::BINARY));
  EXPECT_EQ(static_cast<int>(root.binaryOperator),
            static_cast<int>(BinaryOperator::SUBTRACT));

  Expression right = at(parser, root.rightExprIndex);
  EXPECT_DOUBLE_EQ(right.numValue, 3.0);

  Expression left = at(parser, root.leftExprIndex);
  EXPECT_EQ(static_cast<int>(left.kind),
            static_cast<int>(ExpressionKind::BINARY));
  EXPECT_EQ(static_cast<int>(left.binaryOperator),
            static_cast<int>(BinaryOperator::SUBTRACT));
  EXPECT_DOUBLE_EQ(at(parser, left.leftExprIndex).numValue, 1.0);
  EXPECT_DOUBLE_EQ(at(parser, left.rightExprIndex).numValue, 2.0);
  EXPECT_TRUE(parser.errors.empty());
}

TEST(Parser, ParseBinarySumBindsTighterThanComparison) {
  std::vector<Token> tokens = tokenize("1 + 2 == 3;");
  Parser parser{tokens};

  parser.parseExpressionStatment();

  ASSERT_FALSE(parser.parserResult.statements.empty());
  // Expect (1 + 2) == 3: root is EQUAL, left child is ADD.
  Expression root =
      at(parser, parser.parserResult.statements[0].expressionIndex);
  EXPECT_EQ(static_cast<int>(root.kind),
            static_cast<int>(ExpressionKind::BINARY));
  EXPECT_EQ(static_cast<int>(root.binaryOperator),
            static_cast<int>(BinaryOperator::EQUAL));

  Expression right = at(parser, root.rightExprIndex);
  EXPECT_DOUBLE_EQ(right.numValue, 3.0);

  Expression left = at(parser, root.leftExprIndex);
  EXPECT_EQ(static_cast<int>(left.kind),
            static_cast<int>(ExpressionKind::BINARY));
  EXPECT_EQ(static_cast<int>(left.binaryOperator),
            static_cast<int>(BinaryOperator::ADD));
  EXPECT_DOUBLE_EQ(at(parser, left.leftExprIndex).numValue, 1.0);
  EXPECT_DOUBLE_EQ(at(parser, left.rightExprIndex).numValue, 2.0);
  EXPECT_TRUE(parser.errors.empty());
}

} // namespace
