#include "interpreter/evaluator.h"
#include "interpreter/value.h"
#include "lexer/lexer.h"
#include "lexer/token.h"
#include "parser/parser.h"

#include <gtest/gtest.h>

#include <cstdio>
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

Value evalCapturingOutput(const std::string &input, std::string &output) {
  testing::internal::CaptureStdout();
  Value value = eval(input);
  std::fflush(stdout);
  output = testing::internal::GetCapturedStdout();
  return value;
}

std::string evalOutput(const std::string &input) {
  std::string output;
  evalCapturingOutput(input, output);
  return output;
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

TEST(Evaluator, EvalArrayLiteralEmpty) {
  Value value = eval("[];");

  EXPECT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Array));
  EXPECT_TRUE(value.values.empty());
}

TEST(Evaluator, EvalArrayLiteralWithValues) {
  Value value = eval("[1, \"a\", true];");

  ASSERT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Array));
  ASSERT_EQ(value.values.size(), 3u);

  EXPECT_EQ(static_cast<int>(value.values[0]->kind),
            static_cast<int>(ValueKind::Number));
  EXPECT_DOUBLE_EQ(value.values[0]->numValue, 1.0);

  EXPECT_EQ(static_cast<int>(value.values[1]->kind),
            static_cast<int>(ValueKind::String));
  EXPECT_EQ(value.values[1]->strValue, "a");

  EXPECT_EQ(static_cast<int>(value.values[2]->kind),
            static_cast<int>(ValueKind::Bool));
  EXPECT_TRUE(value.values[2]->boolValue);
}

TEST(Evaluator, EvalArrayLiteralEvaluatesElements) {
  Value value = eval("[1 + 2, 3 * 4];");

  ASSERT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Array));
  ASSERT_EQ(value.values.size(), 2u);
  EXPECT_DOUBLE_EQ(value.values[0]->numValue, 3.0);
  EXPECT_DOUBLE_EQ(value.values[1]->numValue, 12.0);
}

TEST(Evaluator, EvalArrayLiteralStoredInVar) {
  Value value = eval("var arr = [1, 2, 3]; arr;");

  ASSERT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Array));
  ASSERT_EQ(value.values.size(), 3u);
  EXPECT_DOUBLE_EQ(value.values[0]->numValue, 1.0);
  EXPECT_DOUBLE_EQ(value.values[1]->numValue, 2.0);
  EXPECT_DOUBLE_EQ(value.values[2]->numValue, 3.0);
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

TEST(Evaluator, EvalArrayAddConcatenates) {
  Value value = eval("[1, 2] + [3, 4];");

  ASSERT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Array));
  ASSERT_EQ(value.values.size(), 4u);
  EXPECT_DOUBLE_EQ(value.values[0]->numValue, 1.0);
  EXPECT_DOUBLE_EQ(value.values[1]->numValue, 2.0);
  EXPECT_DOUBLE_EQ(value.values[2]->numValue, 3.0);
  EXPECT_DOUBLE_EQ(value.values[3]->numValue, 4.0);
}

TEST(Evaluator, EvalArrayAddEmptyArrays) {
  Value value = eval("[] + [];");

  EXPECT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Array));
  EXPECT_TRUE(value.values.empty());
}

TEST(Evaluator, EvalArrayAddWithEmptyArray) {
  Value value = eval("[1] + [];");

  ASSERT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Array));
  ASSERT_EQ(value.values.size(), 1u);
  EXPECT_DOUBLE_EQ(value.values[0]->numValue, 1.0);
}

TEST(Evaluator, EvalArrayAddPreservesOrder) {
  Value value = eval("[\"a\", \"b\"] + [\"c\"];");

  ASSERT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Array));
  ASSERT_EQ(value.values.size(), 3u);
  EXPECT_EQ(value.values[0]->strValue, "a");
  EXPECT_EQ(value.values[1]->strValue, "b");
  EXPECT_EQ(value.values[2]->strValue, "c");
}

TEST(Evaluator, EvalArraySubtractIsError) {
  Value value = eval("[1] - [2];");

  EXPECT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Error));
  EXPECT_EQ(value.strValue, "unknown operator: Array - Array");
}

TEST(Evaluator, EvalArrayAddNumberIsError) {
  Value value = eval("[1] + 2;");

  EXPECT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Error));
  EXPECT_EQ(value.strValue, "unknown operator: Array + Number");
}

TEST(Evaluator, EvalArrayIndexReturnsElement) {
  Value value = eval("var arr = [1, 2, 3]; arr[1];");

  ASSERT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Number));
  EXPECT_DOUBLE_EQ(value.numValue, 2.0);
}

TEST(Evaluator, EvalArrayIndexFirstElement) {
  Value value = eval("var arr = [\"a\", \"b\"]; arr[0];");

  ASSERT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::String));
  EXPECT_EQ(value.strValue, "a");
}

TEST(Evaluator, EvalArrayIndexOutOfBoundsIsError) {
  Value value = eval("var arr = [1, 2]; arr[2];");

  EXPECT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Error));
  EXPECT_EQ(value.strValue, "index is bigger than array size");
}

TEST(Evaluator, EvalArrayIndexNegativeIsError) {
  Value value = eval("var arr = [1, 2]; arr[-1];");

  EXPECT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Error));
  EXPECT_EQ(value.strValue, "index must be greater or equal than zero");
}

TEST(Evaluator, EvalArrayIndexOnNonArrayIsError) {
  Value value = eval("var notArr = 5; notArr[0];");

  EXPECT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Error));
  EXPECT_EQ(value.strValue, "variable is not an array");
}

TEST(Evaluator, EvalArrayIndexNonNumberIsError) {
  Value value = eval("var arr = [1, 2]; arr[true];");

  EXPECT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Error));
  EXPECT_EQ(value.strValue, "index must be a number");
}

TEST(Evaluator, EvalArrayIndexWithBinaryExpression) {
  Value value = eval("var arr = [1, 2, 3]; arr[1 + 1];");

  ASSERT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Number));
  EXPECT_DOUBLE_EQ(value.numValue, 3.0);
}

TEST(Evaluator, EvalArrayIndexWithVariable) {
  Value value = eval("var arr = [1, 2, 3]; var i = 2; arr[i];");

  ASSERT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Number));
  EXPECT_DOUBLE_EQ(value.numValue, 3.0);
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

TEST(Evaluator, EvalVarStatementReturnsInitializerValue) {
  Value value = eval("var x = 5;");

  EXPECT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Number));
  EXPECT_DOUBLE_EQ(value.numValue, 5.0);
}

TEST(Evaluator, EvalVarStatementBindsName) {
  Value value = eval("var x = 5; x;");

  EXPECT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Number));
  EXPECT_DOUBLE_EQ(value.numValue, 5.0);
}

TEST(Evaluator, EvalVarStatementBindsString) {
  Value value = eval("var greeting = \"hello\"; greeting;");

  EXPECT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::String));
  EXPECT_EQ(value.strValue, "hello");
}

TEST(Evaluator, EvalVarStatementBindsBoolean) {
  Value value = eval("var flag = false; flag;");

  EXPECT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Bool));
  EXPECT_FALSE(value.boolValue);
}

TEST(Evaluator, EvalVarStatementEvaluatesInitializer) {
  Value value = eval("var x = 2 * 3 + 4; x;");

  EXPECT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Number));
  EXPECT_DOUBLE_EQ(value.numValue, 10.0);
}

TEST(Evaluator, EvalVarStatementInitializerSeesEarlierNames) {
  Value value = eval("var a = 1; var b = a + 2; var c = a + b; c;");

  EXPECT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Number));
  EXPECT_DOUBLE_EQ(value.numValue, 4.0);
}

TEST(Evaluator, EvalVarStatementRebindingOverwrites) {
  Value value = eval("var x = 5; var x = 9; x;");

  EXPECT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Number));
  EXPECT_DOUBLE_EQ(value.numValue, 9.0);
}

TEST(Evaluator, EvalVarStatementRebindingChangesKind) {
  Value value = eval("var x = 5; var x = \"five\"; x;");

  EXPECT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::String));
  EXPECT_EQ(value.strValue, "five");
}

TEST(Evaluator, EvalVarStatementNameIsUsableInExpressions) {
  Value value = eval("var x = 5; x * 2;");

  EXPECT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Number));
  EXPECT_DOUBLE_EQ(value.numValue, 10.0);
}

TEST(Evaluator, EvalVarStatementNameIsUsableAsIfCondition) {
  Value value = eval("var flag = false; if (flag) { 1; } else { 2; }");

  EXPECT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Number));
  EXPECT_DOUBLE_EQ(value.numValue, 2.0);
}

TEST(Evaluator, EvalVarStatementWithoutInitializerIsNull) {
  Value value = eval("var x;");

  EXPECT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Null));
}

TEST(Evaluator, EvalVarStatementWithoutInitializerBindsNull) {
  Value value = eval("var x; x;");

  EXPECT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Null));
}

TEST(Evaluator, EvalVarStatementErrorInitializerStopsProgram) {
  Value value = eval("var x = -true; 5;");

  EXPECT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Error));
  EXPECT_EQ(value.strValue, "unknown operator: -Bool");
}

TEST(Evaluator, EvalVarStatementInsideBlockIsVisibleOutside) {
  Value value = eval("if (true) { var x = 7; } x;");

  EXPECT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Number));
  EXPECT_DOUBLE_EQ(value.numValue, 7.0);
}

TEST(Evaluator, EvalUnboundNameIsError) {
  Value value = eval("nope;");

  EXPECT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Error));
  EXPECT_EQ(value.strValue, "identifier not found: nope");
}

TEST(Evaluator, EvalCallExpressionAddsArgs) {
  Value value = eval("var add = fn(x, y) { x + y; }; add(1, 2);");

  EXPECT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Number));
  EXPECT_DOUBLE_EQ(value.numValue, 3.0);
}

TEST(Evaluator, EvalCallExpressionMultipleBodyStatementsReturnsLast) {
  Value value = eval("var f = fn(x) { var y = x + 1; y * 2; }; f(3);");

  EXPECT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Number));
  EXPECT_DOUBLE_EQ(value.numValue, 8.0);
}

TEST(Evaluator, EvalCallExpressionArgumentsAreLexicallyScoped) {
  Value value = eval("var x = 10; var f = fn(x) { x; }; f(5); x;");

  EXPECT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Number));
  EXPECT_DOUBLE_EQ(value.numValue, 10.0);
}

TEST(Evaluator, EvalCallExpressionReturnStopsBodyEarly) {
  Value value = eval("var f = fn(x) { return x + 1; x + 100; }; f(2);");

  EXPECT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Number));
  EXPECT_DOUBLE_EQ(value.numValue, 3.0);
}

TEST(Evaluator, EvalCallExpressionArgumentCountMismatchIsError) {
  Value value = eval("var add = fn(x, y) { x + y; }; add(1);");

  EXPECT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Error));
  EXPECT_EQ(value.strValue, "wrong number of arguments: got 1, want 2");
}

TEST(Evaluator, EvalBuiltinPrintString) {
  EXPECT_EQ(evalOutput("print(\"hello\");"), "hello\n");
}

TEST(Evaluator, EvalBuiltinPrintNumber) {
  EXPECT_EQ(evalOutput("print(1 + 2);"), "3\n");
}

TEST(Evaluator, EvalBuiltinPrintBool) {
  EXPECT_EQ(evalOutput("print(true);"), "true\n");
}

TEST(Evaluator, EvalBuiltinPrintNull) {
  EXPECT_EQ(evalOutput("var x; print(x);"), "null\n");
}

TEST(Evaluator, EvalBuiltinPrintWithoutArgs) {
  EXPECT_EQ(evalOutput("print();"), "\n");
}

TEST(Evaluator, EvalBuiltinPrintConcatenatesArgs) {
  EXPECT_EQ(evalOutput("print(\"a\", 1, false);"), "a1false\n");
}

TEST(Evaluator, EvalBuiltinPrintEvaluatesArgs) {
  EXPECT_EQ(evalOutput("var name = \"ayga\"; print(\"hi \" + name);"),
            "hi ayga\n");
}

TEST(Evaluator, EvalBuiltinPrintEveryCallOutputs) {
  EXPECT_EQ(evalOutput("print(\"a\"); print(\"b\");"), "a\nb\n");
}

TEST(Evaluator, EvalBuiltinPrintInsideFunctionBody) {
  EXPECT_EQ(evalOutput("var f = fn(x) { print(x); }; f(7);"), "7\n");
}

TEST(Evaluator, EvalBuiltinPrintReturnsNull) {
  std::string output;
  Value value = evalCapturingOutput("print(\"hello\");", output);

  EXPECT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Null));
}

TEST(Evaluator, EvalBuiltinPrintArgumentErrorIsReturned) {
  std::string output;
  Value value = evalCapturingOutput("print(-true);", output);

  EXPECT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Error));
  EXPECT_EQ(value.strValue, "unknown operator: -Bool");
  EXPECT_EQ(output, "");
}

TEST(Evaluator, EvalBuiltinPrintUnboundArgumentIsError) {
  std::string output;
  Value value = evalCapturingOutput("print(nope);", output);

  EXPECT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Error));
  EXPECT_EQ(value.strValue, "identifier not found: nope");
  EXPECT_EQ(output, "");
}

TEST(Evaluator, EvalBuiltinPrintIsNotShadowedByVar) {
  EXPECT_EQ(evalOutput("var print = 5; print(\"hello\");"), "hello\n");
}

TEST(Evaluator, EvalBuiltinPrintEmptyArray) {
  EXPECT_EQ(evalOutput("print([]);"), "[]\n");
}

TEST(Evaluator, EvalBuiltinPrintArrayOfNumbers) {
  EXPECT_EQ(evalOutput("print([1, 2, 3]);"), "[1,2,3]\n");
}

TEST(Evaluator, EvalBuiltinPrintArrayOfStrings) {
  EXPECT_EQ(evalOutput("print([\"a\", \"b\"]);"), "[a,b]\n");
}

TEST(Evaluator, EvalBuiltinPrintArrayOfBooleans) {
  EXPECT_EQ(evalOutput("print([true, false]);"), "[true,false]\n");
}

TEST(Evaluator, EvalBuiltinLenString) {
  Value value = eval("len(\"hello\");");

  EXPECT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Number));
  EXPECT_DOUBLE_EQ(value.numValue, 5.0);
}

TEST(Evaluator, EvalBuiltinLenEmptyString) {
  Value value = eval("len(\"\");");

  EXPECT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Number));
  EXPECT_DOUBLE_EQ(value.numValue, 0.0);
}

TEST(Evaluator, EvalBuiltinLenEvaluatesArgs) {
  Value value = eval("var name = \"ayga\"; len(\"hi \" + name);");

  EXPECT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Number));
  EXPECT_DOUBLE_EQ(value.numValue, 7.0);
}

TEST(Evaluator, EvalBuiltinLenIsNotShadowedByVar) {
  Value value = eval("var len = 5; len(\"abc\");");

  EXPECT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Number));
  EXPECT_DOUBLE_EQ(value.numValue, 3.0);
}

TEST(Evaluator, EvalBuiltinLenWithoutArgsIsError) {
  std::string output;
  Value value = evalCapturingOutput("len();", output);

  EXPECT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Error));
  EXPECT_EQ(value.strValue, "wrong number of arguments: got 0, want 1");
  EXPECT_EQ(output, "");
}

TEST(Evaluator, EvalBuiltinLenTooManyArgsIsError) {
  std::string output;
  Value value = evalCapturingOutput("len(\"a\", \"b\");", output);

  EXPECT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Error));
  EXPECT_EQ(value.strValue, "wrong number of arguments: got 2, want 1");
  EXPECT_EQ(output, "");
}

TEST(Evaluator, EvalBuiltinLenNumberArgumentIsError) {
  std::string output;
  Value value = evalCapturingOutput("len(1);", output);

  EXPECT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Error));
  EXPECT_EQ(value.strValue, "argument to len is not supported: Number");
  EXPECT_EQ(output, "");
}

TEST(Evaluator, EvalBuiltinLenBoolArgumentIsError) {
  std::string output;
  Value value = evalCapturingOutput("len(true);", output);

  EXPECT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Error));
  EXPECT_EQ(value.strValue, "argument to len is not supported: Bool");
  EXPECT_EQ(output, "");
}

TEST(Evaluator, EvalBuiltinLenNullArgumentIsError) {
  std::string output;
  Value value = evalCapturingOutput("var x; len(x);", output);

  EXPECT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Error));
  EXPECT_EQ(value.strValue, "argument to len is not supported: Null");
  EXPECT_EQ(output, "");
}

TEST(Evaluator, EvalBuiltinLenArgumentErrorIsReturned) {
  std::string output;
  Value value = evalCapturingOutput("len(-true);", output);

  EXPECT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Error));
  EXPECT_EQ(value.strValue, "unknown operator: -Bool");
  EXPECT_EQ(output, "");
}

TEST(Evaluator, EvalBuiltinLenUnboundArgumentIsError) {
  std::string output;
  Value value = evalCapturingOutput("len(nope);", output);

  EXPECT_EQ(static_cast<int>(value.kind), static_cast<int>(ValueKind::Error));
  EXPECT_EQ(value.strValue, "identifier not found: nope");
  EXPECT_EQ(output, "");
}

} // namespace
