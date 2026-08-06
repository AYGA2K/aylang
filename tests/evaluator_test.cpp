#include "interpreter/evaluator.h"
#include "interpreter/value.h"
#include "lexer/lexer.h"
#include "lexer/token.h"
#include "parser/parser.h"

#include <gtest/gtest.h>

#include <string>
#include <vector>

namespace {

Value eval(const std::string &input) {
  Lexer lexer{.input = input};
  std::vector<Token> tokens = lexer.tokenize();
  Parser parser{tokens};
  parser.parse();
  Evaluator evaluator{.parserResult = parser.parserResult};
  return evaluator.evalStatements();
}

TEST(Evaluator, EvalNumberInteger) {
  Value value = eval("42;");

  EXPECT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Number));
  EXPECT_DOUBLE_EQ(value.numValue, 42.0);
}

TEST(Evaluator, EvalNumberFloat) {
  Value value = eval("3.14;");

  EXPECT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Number));
  EXPECT_DOUBLE_EQ(value.numValue, 3.14);
}

TEST(Evaluator, EvalBooleanTrue) {
  Value value = eval("true;");

  EXPECT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Bool));
  EXPECT_TRUE(value.boolValue);
}

TEST(Evaluator, EvalBooleanFalse) {
  Value value = eval("false;");

  EXPECT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Bool));
  EXPECT_FALSE(value.boolValue);
}

TEST(Evaluator, EvalStringLiteral) {
  Value value = eval("\"hello\";");

  EXPECT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::String));
  EXPECT_EQ(value.strValue, "hello");
}

TEST(Evaluator, EvalEmptyStringLiteral) {
  Value value = eval("\"\";");

  EXPECT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::String));
  EXPECT_EQ(value.strValue, "");
}

TEST(Evaluator, EvalLastStatement) {
  Value value = eval("1; 2; 3;");

  EXPECT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Number));
  EXPECT_DOUBLE_EQ(value.numValue, 3.0);
}

TEST(Evaluator, EvalUnaryMinus) {
  Value value = eval("-5;");

  EXPECT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Number));
  EXPECT_DOUBLE_EQ(value.numValue, -5.0);
}

TEST(Evaluator, EvalUnaryBang) {
  Value value = eval("!true;");

  EXPECT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Bool));
  EXPECT_FALSE(value.boolValue);
}

TEST(Evaluator, EvalNumberEqual) {
  Value value = eval("1 == 1;");

  EXPECT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Bool));
  EXPECT_TRUE(value.boolValue);
}

TEST(Evaluator, EvalNumberEqualDifferentNumbers) {
  Value value = eval("1 == 2;");

  EXPECT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Bool));
  EXPECT_FALSE(value.boolValue);
}

TEST(Evaluator, EvalNumberNotEqual) {
  Value value = eval("1 != 2;");

  EXPECT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Bool));
  EXPECT_TRUE(value.boolValue);
}

TEST(Evaluator, EvalNumberNotEqualSameNumbers) {
  Value value = eval("1 != 1;");

  EXPECT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Bool));
  EXPECT_FALSE(value.boolValue);
}

TEST(Evaluator, EvalNumberLessThan) {
  Value value = eval("1 < 2;");

  EXPECT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Bool));
  EXPECT_TRUE(value.boolValue);
}

TEST(Evaluator, EvalNumberLessThanGreaterLeft) {
  Value value = eval("2 < 1;");

  EXPECT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Bool));
  EXPECT_FALSE(value.boolValue);
}

TEST(Evaluator, EvalNumberGreaterThan) {
  Value value = eval("2 > 1;");

  EXPECT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Bool));
  EXPECT_TRUE(value.boolValue);
}

TEST(Evaluator, EvalNumberGreaterThanSmallerLeft) {
  Value value = eval("1 > 2;");

  EXPECT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Bool));
  EXPECT_FALSE(value.boolValue);
}

TEST(Evaluator, EvalNumberLessThanOrEqualEqualNumbers) {
  Value value = eval("1 <= 1;");

  EXPECT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Bool));
  EXPECT_TRUE(value.boolValue);
}

TEST(Evaluator, EvalNumberLessThanOrEqualGreaterLeft) {
  Value value = eval("2 <= 1;");

  EXPECT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Bool));
  EXPECT_FALSE(value.boolValue);
}

TEST(Evaluator, EvalNumberGreaterThanOrEqualEqualNumbers) {
  Value value = eval("2 >= 2;");

  EXPECT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Bool));
  EXPECT_TRUE(value.boolValue);
}

TEST(Evaluator, EvalNumberGreaterThanOrEqualSmallerLeft) {
  Value value = eval("1 >= 2;");

  EXPECT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Bool));
  EXPECT_FALSE(value.boolValue);
}

TEST(Evaluator, EvalNegativeNumberLessThan) {
  Value value = eval("-1 < 0;");

  EXPECT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Bool));
  EXPECT_TRUE(value.boolValue);
}

TEST(Evaluator, EvalNegativeNumberEqual) {
  Value value = eval("-2 == -2;");

  EXPECT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Bool));
  EXPECT_TRUE(value.boolValue);
}

TEST(Evaluator, EvalComparisonResultEqualBoolean) {
  Value value = eval("1 < 2 == true;");

  EXPECT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Bool));
  EXPECT_TRUE(value.boolValue);
}

TEST(Evaluator, EvalBooleanEqual) {
  Value value = eval("true == true;");

  EXPECT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Bool));
  EXPECT_TRUE(value.boolValue);
}

TEST(Evaluator, EvalBooleanNotEqual) {
  Value value = eval("true != false;");

  EXPECT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Bool));
  EXPECT_TRUE(value.boolValue);
}

TEST(Evaluator, EvalStringEqualSameString) {
  Value value = eval("\"foo\" == \"foo\";");

  EXPECT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Bool));
  EXPECT_TRUE(value.boolValue);
}

TEST(Evaluator, EvalStringEqualDifferentString) {
  Value value = eval("\"foo\" == \"bar\";");

  EXPECT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Bool));
  EXPECT_FALSE(value.boolValue);
}

TEST(Evaluator, EvalStringNotEqualDifferentString) {
  Value value = eval("\"foo\" != \"bar\";");

  EXPECT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Bool));
  EXPECT_TRUE(value.boolValue);
}

TEST(Evaluator, EvalBooleanLessThan) {
  Value value = eval("false < true;");

  EXPECT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Bool));
  EXPECT_TRUE(value.boolValue);
}

TEST(Evaluator, EvalNumberEqualTrue) {
  Value value = eval("1 == true;");

  EXPECT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Bool));
  EXPECT_TRUE(value.boolValue);
}

TEST(Evaluator, EvalNumberEqualFalse) {
  Value value = eval("0 == false;");

  EXPECT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Bool));
  EXPECT_TRUE(value.boolValue);
}

TEST(Evaluator, EvalTrueEqualNumber) {
  Value value = eval("true == 1;");

  EXPECT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Bool));
  EXPECT_TRUE(value.boolValue);
}

TEST(Evaluator, EvalNumberEqualTrueOtherNumber) {
  Value value = eval("2 == true;");

  EXPECT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Bool));
  EXPECT_FALSE(value.boolValue);
}

TEST(Evaluator, EvalNumberNotEqualFalse) {
  Value value = eval("1 != false;");

  EXPECT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Bool));
  EXPECT_TRUE(value.boolValue);
}

TEST(Evaluator, EvalNumberGreaterThanTrue) {
  Value value = eval("2 > true;");

  EXPECT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Bool));
  EXPECT_TRUE(value.boolValue);
}

TEST(Evaluator, EvalFalseLessThanNumber) {
  Value value = eval("false < 1;");

  EXPECT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Bool));
  EXPECT_TRUE(value.boolValue);
}

TEST(Evaluator, EvalTrueGreaterThanNumber) {
  Value value = eval("true > 2;");

  EXPECT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Bool));
  EXPECT_FALSE(value.boolValue);
}

TEST(Evaluator, EvalNegativeNumberEqualTrue) {
  Value value = eval("-1 == true;");

  EXPECT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Bool));
  EXPECT_FALSE(value.boolValue);
}

TEST(Evaluator, EvalNegativeNumberLessThanFalse) {
  Value value = eval("-1 < false;");

  EXPECT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Bool));
  EXPECT_TRUE(value.boolValue);
}

TEST(Evaluator, EvalNegativeZeroEqualZero) {
  Value value = eval("-0 == 0;");

  EXPECT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Bool));
  EXPECT_TRUE(value.boolValue);
}

TEST(Evaluator, EvalNegativeZeroEqualFalse) {
  Value value = eval("-0 == false;");

  EXPECT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Bool));
  EXPECT_TRUE(value.boolValue);
}

TEST(Evaluator, EvalIntegerEqualFloat) {
  Value value = eval("1 == 1.0;");

  EXPECT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Bool));
  EXPECT_TRUE(value.boolValue);
}

TEST(Evaluator, EvalFloatEqualFalse) {
  Value value = eval("0.0 == false;");

  EXPECT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Bool));
  EXPECT_TRUE(value.boolValue);
}

TEST(Evaluator, EvalFractionEqualFalse) {
  Value value = eval("0.5 == false;");

  EXPECT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Bool));
  EXPECT_FALSE(value.boolValue);
}

TEST(Evaluator, EvalFractionLessThanTrue) {
  Value value = eval("0.5 < true;");

  EXPECT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Bool));
  EXPECT_TRUE(value.boolValue);
}

TEST(Evaluator, EvalFractionGreaterThanFalse) {
  Value value = eval("0.5 > false;");

  EXPECT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Bool));
  EXPECT_TRUE(value.boolValue);
}

TEST(Evaluator, EvalCloseFloatsLessThan) {
  Value value = eval("3.14 < 3.15;");

  EXPECT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Bool));
  EXPECT_TRUE(value.boolValue);
}

TEST(Evaluator, EvalChainedLessThan) {
  Value value = eval("1 < 2 < 3;");

  EXPECT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Bool));
  EXPECT_TRUE(value.boolValue);
}

TEST(Evaluator, EvalChainedLessThanSmallerRight) {
  Value value = eval("1 < 2 < 1;");

  EXPECT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Bool));
  EXPECT_FALSE(value.boolValue);
}

TEST(Evaluator, EvalChainedGreaterThan) {
  Value value = eval("2 > 1 > 0;");

  EXPECT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Bool));
  EXPECT_TRUE(value.boolValue);
}

TEST(Evaluator, EvalComparisonEqualBooleanChain) {
  Value value = eval("2 > true == true;");

  EXPECT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Bool));
  EXPECT_TRUE(value.boolValue);
}

TEST(Evaluator, EvalUnaryBangGroupedComparison) {
  Value value = eval("!(1 < 2);");

  EXPECT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Bool));
  EXPECT_FALSE(value.boolValue);
}

TEST(Evaluator, EvalUnaryBangGroupedFalseComparison) {
  Value value = eval("!(1 > 2);");

  EXPECT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Bool));
  EXPECT_TRUE(value.boolValue);
}

TEST(Evaluator, EvalDoubleUnaryBangFalse) {
  Value value = eval("!!false;");

  EXPECT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Bool));
  EXPECT_FALSE(value.boolValue);
}

TEST(Evaluator, EvalDoubleUnaryMinus) {
  Value value = eval("--5;");

  EXPECT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Number));
  EXPECT_DOUBLE_EQ(value.numValue, 5.0);
}

TEST(Evaluator, EvalReturnStatement) {
  Value value = eval("return 10;");

  EXPECT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Number));
  EXPECT_DOUBLE_EQ(value.numValue, 10.0);
}

TEST(Evaluator, EvalReturnStatementExpression) {
  Value value = eval("return 2 * 5;");

  EXPECT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Number));
  EXPECT_DOUBLE_EQ(value.numValue, 10.0);
}

TEST(Evaluator, EvalReturnStatementStopsFollowingStatements) {
  Value value = eval("return 10; 9;");

  EXPECT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Number));
  EXPECT_DOUBLE_EQ(value.numValue, 10.0);
}

TEST(Evaluator, EvalReturnStatementAfterOtherStatements) {
  Value value = eval("9; return 2 * 5; 9;");

  EXPECT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Number));
  EXPECT_DOUBLE_EQ(value.numValue, 10.0);
}

TEST(Evaluator, EvalReturnStatementInsideIfWithMultipleStatements) {
  Value value = eval("if ((1000 / 2) + 250 * 2 == 1000) { 9999; return 87; 90; }");

  EXPECT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Number));
  EXPECT_DOUBLE_EQ(value.numValue, 87.0);
}

TEST(Evaluator, EvalUnaryMinusOnBooleanIsError) {
  Value value = eval("-true;");

  EXPECT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Error));
  EXPECT_EQ(value.strValue, "unknown operator: -Bool");
}

TEST(Evaluator, EvalUnaryBangOnNumberIsError) {
  Value value = eval("!5;");

  EXPECT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Error));
  EXPECT_EQ(value.strValue, "unknown operator: !Number");
}

TEST(Evaluator, EvalAddNumberAndErrorIsError) {
  Value value = eval("5 + (-true);");

  EXPECT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Error));
  EXPECT_EQ(value.strValue, "unknown operator: Number + Error");
}

TEST(Evaluator, EvalMultiplyNumberAndErrorIsError) {
  Value value = eval("2 * (-true);");

  EXPECT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Error));
  EXPECT_EQ(value.strValue, "unknown operator: Number * Error");
}

TEST(Evaluator, EvalStringAddConcatenates) {
  Value value = eval("\"foo\" + \"bar\";");

  EXPECT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::String));
  EXPECT_EQ(value.strValue, "foobar");
}

TEST(Evaluator, EvalStringSubtractIsError) {
  Value value = eval("\"foo\" - \"bar\";");

  EXPECT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Error));
  EXPECT_EQ(value.strValue, "unknown operator: Str - Str");
}

TEST(Evaluator, EvalErrorStopsFollowingStatements) {
  Value value = eval("-true; 5;");

  EXPECT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Error));
  EXPECT_EQ(value.strValue, "unknown operator: -Bool");
}

TEST(Evaluator, EvalErrorInsideIfBlockStopsFollowingStatements) {
  Value value = eval("if (true) { -true; 9; }");

  EXPECT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Error));
  EXPECT_EQ(value.strValue, "unknown operator: -Bool");
}

TEST(Evaluator, EvalErrorConditionSkipsIfBranches) {
  Value value = eval("if (-true) { 9; } else { 10; }");

  EXPECT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Error));
  EXPECT_EQ(value.strValue, "unknown operator: -Bool");
}

} // namespace
