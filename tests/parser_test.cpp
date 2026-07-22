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

TEST(Parser, ParseGroupedExpression) {
  std::vector<Token> tokens = tokenize("(5 + 5);");
  Parser parser{tokens};

  int index = parser.parseGroupedExpression();
  Expression expression = at(parser, index);

  EXPECT_EQ(static_cast<int>(expression.kind),
            static_cast<int>(ExpressionKind::BINARY));
  EXPECT_EQ(static_cast<int>(expression.binaryOperator),
            static_cast<int>(BinaryOperator::ADD));
  EXPECT_DOUBLE_EQ(at(parser, expression.leftExprIndex).numValue, 5.0);
  EXPECT_DOUBLE_EQ(at(parser, expression.rightExprIndex).numValue, 5.0);
  EXPECT_TRUE(parser.errors.empty());
}

TEST(Parser, ParseGroupedExpressionMissingClosingParen) {
  std::vector<Token> tokens = tokenize("(5 + 5;");
  Parser parser{tokens};

  int index = parser.parseGroupedExpression();

  EXPECT_EQ(index, -1);
}

TEST(Parser, ParseGroupedExpressionOverridesPrecedence) {
  std::vector<Token> tokens = tokenize("(1 + 2) * 3;");
  Parser parser{tokens};

  parser.parseExpressionStatment();

  ASSERT_FALSE(parser.parserResult.statements.empty());
  // Expect (1 + 2) * 3: root is MULTIPLY, left child is ADD.
  Expression root =
      at(parser, parser.parserResult.statements[0].expressionIndex);
  EXPECT_EQ(static_cast<int>(root.kind),
            static_cast<int>(ExpressionKind::BINARY));
  EXPECT_EQ(static_cast<int>(root.binaryOperator),
            static_cast<int>(BinaryOperator::MULTIPLY));

  Expression left = at(parser, root.leftExprIndex);
  EXPECT_EQ(static_cast<int>(left.kind),
            static_cast<int>(ExpressionKind::BINARY));
  EXPECT_EQ(static_cast<int>(left.binaryOperator),
            static_cast<int>(BinaryOperator::ADD));
  EXPECT_DOUBLE_EQ(at(parser, left.leftExprIndex).numValue, 1.0);
  EXPECT_DOUBLE_EQ(at(parser, left.rightExprIndex).numValue, 2.0);

  Expression right = at(parser, root.rightExprIndex);
  EXPECT_DOUBLE_EQ(right.numValue, 3.0);
  EXPECT_TRUE(parser.errors.empty());
}

TEST(Parser, ParseIfExpression) {
  std::vector<Token> tokens = tokenize("if (x) { y; }");
  Parser parser{tokens};

  int index = parser.parseIfExpression();
  ASSERT_GE(index, 0);
  Expression expression = at(parser, index);

  EXPECT_EQ(static_cast<int>(expression.kind),
            static_cast<int>(ExpressionKind::IF));
  ASSERT_GE(expression.conditionExprIndex, 0);
  Expression condition = at(parser, expression.conditionExprIndex);
  EXPECT_EQ(static_cast<int>(condition.kind),
            static_cast<int>(ExpressionKind::IDENTIFIER));
  EXPECT_EQ(condition.stringValue, "x");

  ASSERT_GE(expression.consquenceStmtIndex, 0);
  Statement block =
      parser.parserResult.statements[expression.consquenceStmtIndex];
  EXPECT_EQ(static_cast<int>(block.kind),
            static_cast<int>(StatementKind::BLOCK));
  ASSERT_EQ(block.statementsIndexes.size(), 1u);

  Statement consequence =
      parser.parserResult.statements[block.statementsIndexes[0]];
  EXPECT_EQ(static_cast<int>(consequence.kind),
            static_cast<int>(StatementKind::EXPRESSION));
  Expression consequenceExpr = at(parser, consequence.expressionIndex);
  EXPECT_EQ(static_cast<int>(consequenceExpr.kind),
            static_cast<int>(ExpressionKind::IDENTIFIER));
  EXPECT_EQ(consequenceExpr.stringValue, "y");
  EXPECT_TRUE(parser.errors.empty());
}

TEST(Parser, ParseIfExpressionWithBinaryCondition) {
  std::vector<Token> tokens = tokenize("if (x < y) { x; }");
  Parser parser{tokens};

  int index = parser.parseIfExpression();
  ASSERT_GE(index, 0);
  Expression expression = at(parser, index);

  EXPECT_EQ(static_cast<int>(expression.kind),
            static_cast<int>(ExpressionKind::IF));
  ASSERT_GE(expression.conditionExprIndex, 0);
  Expression condition = at(parser, expression.conditionExprIndex);
  EXPECT_EQ(static_cast<int>(condition.kind),
            static_cast<int>(ExpressionKind::BINARY));
  EXPECT_EQ(static_cast<int>(condition.binaryOperator),
            static_cast<int>(BinaryOperator::LESS_THAN));
  EXPECT_EQ(at(parser, condition.leftExprIndex).stringValue, "x");
  EXPECT_EQ(at(parser, condition.rightExprIndex).stringValue, "y");
  EXPECT_TRUE(parser.errors.empty());
}

TEST(Parser, ParseIfExpressionMultipleConsequenceStatements) {
  std::vector<Token> tokens = tokenize("if (x) { y; z; }");
  Parser parser{tokens};

  int index = parser.parseIfExpression();
  ASSERT_GE(index, 0);
  Expression expression = at(parser, index);

  ASSERT_GE(expression.consquenceStmtIndex, 0);
  Statement block =
      parser.parserResult.statements[expression.consquenceStmtIndex];
  ASSERT_EQ(block.statementsIndexes.size(), 2u);

  Expression first =
      at(parser, parser.parserResult.statements[block.statementsIndexes[0]]
                     .expressionIndex);
  Expression second =
      at(parser, parser.parserResult.statements[block.statementsIndexes[1]]
                     .expressionIndex);
  EXPECT_EQ(first.stringValue, "y");
  EXPECT_EQ(second.stringValue, "z");
}

TEST(Parser, ParseIfElseExpression) {
  std::vector<Token> tokens = tokenize("if (x) { y; } else { z; }");
  Parser parser{tokens};

  int index = parser.parseIfExpression();
  ASSERT_GE(index, 0);
  Expression expression = at(parser, index);

  EXPECT_EQ(static_cast<int>(expression.kind),
            static_cast<int>(ExpressionKind::IF));

  ASSERT_GE(expression.consquenceStmtIndex, 0);
  Statement consequenceBlock =
      parser.parserResult.statements[expression.consquenceStmtIndex];
  ASSERT_EQ(consequenceBlock.statementsIndexes.size(), 1u);
  Expression consequenceExpr =
      at(parser, parser.parserResult
                     .statements[consequenceBlock.statementsIndexes[0]]
                     .expressionIndex);
  EXPECT_EQ(consequenceExpr.stringValue, "y");

  ASSERT_GE(expression.alternativeStmtIndex, 0);
  Statement alternativeBlock =
      parser.parserResult.statements[expression.alternativeStmtIndex];
  EXPECT_EQ(static_cast<int>(alternativeBlock.kind),
            static_cast<int>(StatementKind::BLOCK));
  ASSERT_EQ(alternativeBlock.statementsIndexes.size(), 1u);
  Expression alternativeExpr =
      at(parser, parser.parserResult
                     .statements[alternativeBlock.statementsIndexes[0]]
                     .expressionIndex);
  EXPECT_EQ(alternativeExpr.stringValue, "z");
  EXPECT_TRUE(parser.errors.empty());
}

TEST(Parser, ParseIfElseExpressionMultipleAlternativeStatements) {
  std::vector<Token> tokens = tokenize("if (x) { y; } else { a; b; }");
  Parser parser{tokens};

  int index = parser.parseIfExpression();
  ASSERT_GE(index, 0);
  Expression expression = at(parser, index);

  ASSERT_GE(expression.alternativeStmtIndex, 0);
  Statement alternativeBlock =
      parser.parserResult.statements[expression.alternativeStmtIndex];
  ASSERT_EQ(alternativeBlock.statementsIndexes.size(), 2u);

  Expression first =
      at(parser, parser.parserResult
                     .statements[alternativeBlock.statementsIndexes[0]]
                     .expressionIndex);
  Expression second =
      at(parser, parser.parserResult
                     .statements[alternativeBlock.statementsIndexes[1]]
                     .expressionIndex);
  EXPECT_EQ(first.stringValue, "a");
  EXPECT_EQ(second.stringValue, "b");
}

TEST(Parser, ParseIfExpressionMissingOpenParen) {
  std::vector<Token> tokens = tokenize("if x) { y; }");
  Parser parser{tokens};

  int index = parser.parseIfExpression();

  EXPECT_EQ(index, -1);
}

TEST(Parser, ParseIfExpressionMissingCloseParen) {
  std::vector<Token> tokens = tokenize("if (x { y; }");
  Parser parser{tokens};

  int index = parser.parseIfExpression();

  EXPECT_EQ(index, -1);
}

TEST(Parser, ParseIfExpressionMissingBlock) {
  std::vector<Token> tokens = tokenize("if (x)");
  Parser parser{tokens};

  int index = parser.parseIfExpression();
  ASSERT_GE(index, 0);
  Expression expression = at(parser, index);

  EXPECT_EQ(expression.consquenceStmtIndex, -1);
}

} // namespace
