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

const std::vector<Value> &items(const Value &value) {
  return isHashMap(value) ? asHashMap(value)->entries : asArray(value)->items;
}

const std::string &text(const Value &value) {
  return isError(value) ? asError(value)->message : asString(value)->chars;
}

TEST(Evaluator, EvalNumberInteger) {
  Value value = eval("42;");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Number));
  EXPECT_DOUBLE_EQ(value.num, 42.0);
}

TEST(Evaluator, EvalNumberFloat) {
  Value value = eval("3.14;");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Number));
  EXPECT_DOUBLE_EQ(value.num, 3.14);
}

TEST(Evaluator, EvalNullLiteral) {
  Value value = eval("null;");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Null));
}

TEST(Evaluator, EvalNullLiteralInVar) {
  Value value = eval("let x = null; x;");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Null));
}

TEST(Evaluator, EvalNullEqualsNull) {
  Value value = eval("null == null;");

  ASSERT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Bool));
  EXPECT_TRUE(value.boolean);
}

TEST(Evaluator, EvalNumberEqualsNullIsFalse) {
  Value value = eval("1 == null;");

  ASSERT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Bool));
  EXPECT_FALSE(value.boolean);
}

TEST(Evaluator, EvalUninitializedVarEqualsNull) {
  Value value = eval("let x; x == null;");

  ASSERT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Bool));
  EXPECT_TRUE(value.boolean);
}

TEST(Evaluator, EvalNullIsNotTruthy) {
  Value value = eval("if (null) { 1; } else { 2; }");

  ASSERT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Number));
  EXPECT_DOUBLE_EQ(value.num, 2.0);
}

TEST(Evaluator, EvalPrintNull) {
  EXPECT_EQ(evalOutput("print(null);"), "null\n");
}

TEST(Evaluator, EvalNullHashKey) {
  Value value = eval("let h = {null: \"n\"}; h[null];");

  ASSERT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::String));
  EXPECT_EQ(text(value), "n");
}

TEST(Evaluator, EvalBooleanTrue) {
  Value value = eval("true;");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Bool));
  EXPECT_TRUE(value.boolean);
}

TEST(Evaluator, EvalBooleanFalse) {
  Value value = eval("false;");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Bool));
  EXPECT_FALSE(value.boolean);
}

TEST(Evaluator, EvalStringLiteral) {
  Value value = eval("\"hello\";");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::String));
  EXPECT_EQ(text(value), "hello");
}

TEST(Evaluator, EvalEmptyStringLiteral) {
  Value value = eval("\"\";");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::String));
  EXPECT_EQ(text(value), "");
}

TEST(Evaluator, EvalArrayLiteralEmpty) {
  Value value = eval("[];");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Array));
  EXPECT_TRUE(items(value).empty());
}

TEST(Evaluator, EvalArrayLiteralWithValues) {
  Value value = eval("[1, \"a\", true];");

  ASSERT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Array));
  ASSERT_EQ(items(value).size(), 3u);

  EXPECT_EQ(static_cast<int>(kindOf(items(value)[0])),
            static_cast<int>(ValueKind::Number));
  EXPECT_DOUBLE_EQ(items(value)[0].num, 1.0);

  EXPECT_EQ(static_cast<int>(kindOf(items(value)[1])),
            static_cast<int>(ValueKind::String));
  EXPECT_EQ(text(items(value)[1]), "a");

  EXPECT_EQ(static_cast<int>(kindOf(items(value)[2])),
            static_cast<int>(ValueKind::Bool));
  EXPECT_TRUE(items(value)[2].boolean);
}

TEST(Evaluator, EvalArrayLiteralEvaluatesElements) {
  Value value = eval("[1 + 2, 3 * 4];");

  ASSERT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Array));
  ASSERT_EQ(items(value).size(), 2u);
  EXPECT_DOUBLE_EQ(items(value)[0].num, 3.0);
  EXPECT_DOUBLE_EQ(items(value)[1].num, 12.0);
}

TEST(Evaluator, EvalArrayLiteralStoredInVar) {
  Value value = eval("let arr = [1, 2, 3]; arr;");

  ASSERT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Array));
  ASSERT_EQ(items(value).size(), 3u);
  EXPECT_DOUBLE_EQ(items(value)[0].num, 1.0);
  EXPECT_DOUBLE_EQ(items(value)[1].num, 2.0);
  EXPECT_DOUBLE_EQ(items(value)[2].num, 3.0);
}

TEST(Evaluator, EvalHashLiteralEmpty) {
  Value value = eval("{};");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::HashMap));
  EXPECT_TRUE(items(value).empty());
}

TEST(Evaluator, EvalHashLiteralSinglePair) {
  Value value = eval("{\"one\": 1};");

  ASSERT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::HashMap));
  ASSERT_EQ(items(value).size(), 2u);

  EXPECT_EQ(static_cast<int>(kindOf(items(value)[0])),
            static_cast<int>(ValueKind::String));
  EXPECT_EQ(text(items(value)[0]), "one");

  EXPECT_EQ(static_cast<int>(kindOf(items(value)[1])),
            static_cast<int>(ValueKind::Number));
  EXPECT_DOUBLE_EQ(items(value)[1].num, 1.0);
}

TEST(Evaluator, EvalHashLiteralEvaluatesKeysAndValues) {
  Value value = eval("{\"a\" + \"b\": 2 * 3};");

  ASSERT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::HashMap));
  ASSERT_EQ(items(value).size(), 2u);
  EXPECT_EQ(text(items(value)[0]), "ab");
  EXPECT_DOUBLE_EQ(items(value)[1].num, 6.0);
}

TEST(Evaluator, EvalHashLiteralMultiplePairs) {
  Value value = eval("{\"one\": 1, \"two\": 2, \"three\": 3};");

  ASSERT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::HashMap));
  ASSERT_EQ(items(value).size(), 6u);

  EXPECT_EQ(text(items(value)[0]), "one");
  EXPECT_DOUBLE_EQ(items(value)[1].num, 1.0);
  EXPECT_EQ(text(items(value)[2]), "two");
  EXPECT_DOUBLE_EQ(items(value)[3].num, 2.0);
  EXPECT_EQ(text(items(value)[4]), "three");
  EXPECT_DOUBLE_EQ(items(value)[5].num, 3.0);
}

TEST(Evaluator, EvalHashLiteralIdentifierKey) {
  Value value = eval("let k = \"key\"; {k: 1};");

  ASSERT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::HashMap));
  ASSERT_EQ(items(value).size(), 2u);
  EXPECT_EQ(text(items(value)[0]), "key");
  EXPECT_DOUBLE_EQ(items(value)[1].num, 1.0);
}

TEST(Evaluator, EvalHashLiteralNumberKey) {
  Value value = eval("{2: \"x\"};");

  ASSERT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::HashMap));
  ASSERT_EQ(items(value).size(), 2u);
  EXPECT_EQ(static_cast<int>(kindOf(items(value)[0])),
            static_cast<int>(ValueKind::Number));
  EXPECT_DOUBLE_EQ(items(value)[0].num, 2.0);
  EXPECT_EQ(text(items(value)[1]), "x");
}

TEST(Evaluator, EvalHashLiteralBoolKey) {
  Value value = eval("{true: 1};");

  ASSERT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::HashMap));
  ASSERT_EQ(items(value).size(), 2u);
  EXPECT_EQ(static_cast<int>(kindOf(items(value)[0])),
            static_cast<int>(ValueKind::Bool));
  EXPECT_TRUE(items(value)[0].boolean);
  EXPECT_DOUBLE_EQ(items(value)[1].num, 1.0);
}

TEST(Evaluator, EvalHashLiteralArrayKeyIsError) {
  Value value = eval("{[1]: 1};");

  ASSERT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Error));
  EXPECT_EQ(text(value),
            "hash map key must be a number, string, bool or null: Array");
}

TEST(Evaluator, EvalHashLiteralHashKeyIsError) {
  Value value = eval("{{\"a\": 1}: 2};");

  ASSERT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Error));
  EXPECT_EQ(text(value),
            "hash map key must be a number, string, bool or null: HashMap");
}

TEST(Evaluator, EvalHashLiteralFunctionKeyIsError) {
  Value value = eval("{fn(a) { return a; }: 1};");

  ASSERT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Error));
  EXPECT_EQ(text(value),
            "hash map key must be a number, string, bool or null: Function");
}

TEST(Evaluator, EvalHashLiteralSecondPairKeyIsError) {
  Value value = eval("{\"a\": 1, [2]: 3};");

  ASSERT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Error));
  EXPECT_EQ(text(value),
            "hash map key must be a number, string, bool or null: Array");
}

TEST(Evaluator, EvalHashLiteralErrorKeyPropagates) {
  Value value = eval("{nope: 1};");

  ASSERT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Error));
  EXPECT_EQ(text(value), "identifier not found: nope");
}

TEST(Evaluator, EvalHashLiteralNestedHashValue) {
  Value value = eval("{\"a\": {\"b\": 1}};");

  ASSERT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::HashMap));
  ASSERT_EQ(items(value).size(), 2u);
  EXPECT_EQ(text(items(value)[0]), "a");

  const Value &nested = items(value)[1];
  ASSERT_EQ(static_cast<int>(kindOf(nested)),
            static_cast<int>(ValueKind::HashMap));
  ASSERT_EQ(items(nested).size(), 2u);
  EXPECT_EQ(text(items(nested)[0]), "b");
  EXPECT_DOUBLE_EQ(items(nested)[1].num, 1.0);
}

TEST(Evaluator, EvalHashLiteralArrayValue) {
  Value value = eval("{\"a\": [1, 2]};");

  ASSERT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::HashMap));
  ASSERT_EQ(items(value).size(), 2u);

  const Value &array = items(value)[1];
  ASSERT_EQ(static_cast<int>(kindOf(array)), static_cast<int>(ValueKind::Array));
  ASSERT_EQ(items(array).size(), 2u);
  EXPECT_DOUBLE_EQ(items(array)[0].num, 1.0);
  EXPECT_DOUBLE_EQ(items(array)[1].num, 2.0);
}

TEST(Evaluator, EvalHashLiteralStoredInVar) {
  Value value = eval("let h = {\"one\": 1, \"two\": 2}; h;");

  ASSERT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::HashMap));
  ASSERT_EQ(items(value).size(), 4u);

  EXPECT_EQ(text(items(value)[0]), "one");
  EXPECT_DOUBLE_EQ(items(value)[1].num, 1.0);
  EXPECT_EQ(text(items(value)[2]), "two");
  EXPECT_DOUBLE_EQ(items(value)[3].num, 2.0);
}

TEST(Evaluator, EvalHashLiteralInsideArray) {
  Value value = eval("[{\"a\": 1}];");

  ASSERT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Array));
  ASSERT_EQ(items(value).size(), 1u);

  const Value &hash = items(value)[0];
  ASSERT_EQ(static_cast<int>(kindOf(hash)), static_cast<int>(ValueKind::HashMap));
  ASSERT_EQ(items(hash).size(), 2u);
  EXPECT_EQ(text(items(hash)[0]), "a");
  EXPECT_DOUBLE_EQ(items(hash)[1].num, 1.0);
}

TEST(Evaluator, EvalHashLiteralIsTruthy) {
  EXPECT_EQ(eval("if ({}) { 1; } else { 2; }").num, 1.0);
}

TEST(Evaluator, EvalLastStatement) {
  Value value = eval("1; 2; 3;");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Number));
  EXPECT_DOUBLE_EQ(value.num, 3.0);
}

TEST(Evaluator, EvalUnaryMinus) {
  Value value = eval("-5;");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Number));
  EXPECT_DOUBLE_EQ(value.num, -5.0);
}

TEST(Evaluator, EvalUnaryBang) {
  Value value = eval("!true;");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Bool));
  EXPECT_FALSE(value.boolean);
}

TEST(Evaluator, EvalNumberEqual) {
  Value value = eval("1 == 1;");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Bool));
  EXPECT_TRUE(value.boolean);
}

TEST(Evaluator, EvalNumberEqualDifferentNumbers) {
  Value value = eval("1 == 2;");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Bool));
  EXPECT_FALSE(value.boolean);
}

TEST(Evaluator, EvalNumberNotEqual) {
  Value value = eval("1 != 2;");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Bool));
  EXPECT_TRUE(value.boolean);
}

TEST(Evaluator, EvalNumberNotEqualSameNumbers) {
  Value value = eval("1 != 1;");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Bool));
  EXPECT_FALSE(value.boolean);
}

TEST(Evaluator, EvalNumberLessThan) {
  Value value = eval("1 < 2;");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Bool));
  EXPECT_TRUE(value.boolean);
}

TEST(Evaluator, EvalNumberLessThanGreaterLeft) {
  Value value = eval("2 < 1;");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Bool));
  EXPECT_FALSE(value.boolean);
}

TEST(Evaluator, EvalNumberGreaterThan) {
  Value value = eval("2 > 1;");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Bool));
  EXPECT_TRUE(value.boolean);
}

TEST(Evaluator, EvalNumberGreaterThanSmallerLeft) {
  Value value = eval("1 > 2;");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Bool));
  EXPECT_FALSE(value.boolean);
}

TEST(Evaluator, EvalNumberLessThanOrEqualEqualNumbers) {
  Value value = eval("1 <= 1;");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Bool));
  EXPECT_TRUE(value.boolean);
}

TEST(Evaluator, EvalNumberLessThanOrEqualGreaterLeft) {
  Value value = eval("2 <= 1;");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Bool));
  EXPECT_FALSE(value.boolean);
}

TEST(Evaluator, EvalNumberGreaterThanOrEqualEqualNumbers) {
  Value value = eval("2 >= 2;");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Bool));
  EXPECT_TRUE(value.boolean);
}

TEST(Evaluator, EvalNumberGreaterThanOrEqualSmallerLeft) {
  Value value = eval("1 >= 2;");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Bool));
  EXPECT_FALSE(value.boolean);
}

TEST(Evaluator, EvalNegativeNumberLessThan) {
  Value value = eval("-1 < 0;");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Bool));
  EXPECT_TRUE(value.boolean);
}

TEST(Evaluator, EvalNegativeNumberEqual) {
  Value value = eval("-2 == -2;");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Bool));
  EXPECT_TRUE(value.boolean);
}

TEST(Evaluator, EvalComparisonResultEqualBoolean) {
  Value value = eval("1 < 2 == true;");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Bool));
  EXPECT_TRUE(value.boolean);
}

TEST(Evaluator, EvalBooleanEqual) {
  Value value = eval("true == true;");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Bool));
  EXPECT_TRUE(value.boolean);
}

TEST(Evaluator, EvalBooleanNotEqual) {
  Value value = eval("true != false;");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Bool));
  EXPECT_TRUE(value.boolean);
}

TEST(Evaluator, EvalStringEqualSameString) {
  Value value = eval("\"foo\" == \"foo\";");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Bool));
  EXPECT_TRUE(value.boolean);
}

TEST(Evaluator, EvalStringEqualDifferentString) {
  Value value = eval("\"foo\" == \"bar\";");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Bool));
  EXPECT_FALSE(value.boolean);
}

TEST(Evaluator, EvalStringNotEqualDifferentString) {
  Value value = eval("\"foo\" != \"bar\";");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Bool));
  EXPECT_TRUE(value.boolean);
}

TEST(Evaluator, EvalBooleanLessThan) {
  Value value = eval("false < true;");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Bool));
  EXPECT_TRUE(value.boolean);
}

TEST(Evaluator, EvalNumberEqualTrue) {
  Value value = eval("1 == true;");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Bool));
  EXPECT_TRUE(value.boolean);
}

TEST(Evaluator, EvalNumberEqualFalse) {
  Value value = eval("0 == false;");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Bool));
  EXPECT_TRUE(value.boolean);
}

TEST(Evaluator, EvalTrueEqualNumber) {
  Value value = eval("true == 1;");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Bool));
  EXPECT_TRUE(value.boolean);
}

TEST(Evaluator, EvalNumberEqualTrueOtherNumber) {
  Value value = eval("2 == true;");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Bool));
  EXPECT_FALSE(value.boolean);
}

TEST(Evaluator, EvalNumberNotEqualFalse) {
  Value value = eval("1 != false;");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Bool));
  EXPECT_TRUE(value.boolean);
}

TEST(Evaluator, EvalNumberGreaterThanTrue) {
  Value value = eval("2 > true;");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Bool));
  EXPECT_TRUE(value.boolean);
}

TEST(Evaluator, EvalFalseLessThanNumber) {
  Value value = eval("false < 1;");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Bool));
  EXPECT_TRUE(value.boolean);
}

TEST(Evaluator, EvalTrueGreaterThanNumber) {
  Value value = eval("true > 2;");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Bool));
  EXPECT_FALSE(value.boolean);
}

TEST(Evaluator, EvalNegativeNumberEqualTrue) {
  Value value = eval("-1 == true;");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Bool));
  EXPECT_FALSE(value.boolean);
}

TEST(Evaluator, EvalNegativeNumberLessThanFalse) {
  Value value = eval("-1 < false;");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Bool));
  EXPECT_TRUE(value.boolean);
}

TEST(Evaluator, EvalNegativeZeroEqualZero) {
  Value value = eval("-0 == 0;");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Bool));
  EXPECT_TRUE(value.boolean);
}

TEST(Evaluator, EvalNegativeZeroEqualFalse) {
  Value value = eval("-0 == false;");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Bool));
  EXPECT_TRUE(value.boolean);
}

TEST(Evaluator, EvalIntegerEqualFloat) {
  Value value = eval("1 == 1.0;");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Bool));
  EXPECT_TRUE(value.boolean);
}

TEST(Evaluator, EvalFloatEqualFalse) {
  Value value = eval("0.0 == false;");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Bool));
  EXPECT_TRUE(value.boolean);
}

TEST(Evaluator, EvalFractionEqualFalse) {
  Value value = eval("0.5 == false;");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Bool));
  EXPECT_FALSE(value.boolean);
}

TEST(Evaluator, EvalFractionLessThanTrue) {
  Value value = eval("0.5 < true;");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Bool));
  EXPECT_TRUE(value.boolean);
}

TEST(Evaluator, EvalFractionGreaterThanFalse) {
  Value value = eval("0.5 > false;");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Bool));
  EXPECT_TRUE(value.boolean);
}

TEST(Evaluator, EvalCloseFloatsLessThan) {
  Value value = eval("3.14 < 3.15;");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Bool));
  EXPECT_TRUE(value.boolean);
}

TEST(Evaluator, EvalChainedLessThan) {
  Value value = eval("1 < 2 < 3;");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Bool));
  EXPECT_TRUE(value.boolean);
}

TEST(Evaluator, EvalChainedLessThanSmallerRight) {
  Value value = eval("1 < 2 < 1;");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Bool));
  EXPECT_FALSE(value.boolean);
}

TEST(Evaluator, EvalChainedGreaterThan) {
  Value value = eval("2 > 1 > 0;");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Bool));
  EXPECT_TRUE(value.boolean);
}

TEST(Evaluator, EvalComparisonEqualBooleanChain) {
  Value value = eval("2 > true == true;");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Bool));
  EXPECT_TRUE(value.boolean);
}

TEST(Evaluator, EvalUnaryBangGroupedComparison) {
  Value value = eval("!(1 < 2);");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Bool));
  EXPECT_FALSE(value.boolean);
}

TEST(Evaluator, EvalUnaryBangGroupedFalseComparison) {
  Value value = eval("!(1 > 2);");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Bool));
  EXPECT_TRUE(value.boolean);
}

TEST(Evaluator, EvalDoubleUnaryBangFalse) {
  Value value = eval("!!false;");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Bool));
  EXPECT_FALSE(value.boolean);
}

TEST(Evaluator, EvalDoubleUnaryMinus) {
  Value value = eval("--5;");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Number));
  EXPECT_DOUBLE_EQ(value.num, 5.0);
}

TEST(Evaluator, EvalReturnStatement) {
  Value value = eval("return 10;");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Number));
  EXPECT_DOUBLE_EQ(value.num, 10.0);
}

TEST(Evaluator, EvalReturnStatementExpression) {
  Value value = eval("return 2 * 5;");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Number));
  EXPECT_DOUBLE_EQ(value.num, 10.0);
}

TEST(Evaluator, EvalReturnStatementStopsFollowingStatements) {
  Value value = eval("return 10; 9;");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Number));
  EXPECT_DOUBLE_EQ(value.num, 10.0);
}

TEST(Evaluator, EvalReturnStatementAfterOtherStatements) {
  Value value = eval("9; return 2 * 5; 9;");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Number));
  EXPECT_DOUBLE_EQ(value.num, 10.0);
}

TEST(Evaluator, EvalReturnStatementInsideIfWithMultipleStatements) {
  Value value = eval("if ((1000 / 2) + 250 * 2 == 1000) { 9999; return 87; 90; }");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Number));
  EXPECT_DOUBLE_EQ(value.num, 87.0);
}

TEST(Evaluator, EvalUnaryMinusOnBooleanIsError) {
  Value value = eval("-true;");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Error));
  EXPECT_EQ(text(value), "unknown operator: -Bool");
}

TEST(Evaluator, EvalUnaryBangOnNumberIsError) {
  Value value = eval("!5;");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Error));
  EXPECT_EQ(text(value), "unknown operator: !Number");
}

TEST(Evaluator, EvalAddNumberAndErrorIsError) {
  Value value = eval("5 + (-true);");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Error));
  EXPECT_EQ(text(value), "unknown operator: -Bool");
}

TEST(Evaluator, EvalMultiplyNumberAndErrorIsError) {
  Value value = eval("2 * (-true);");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Error));
  EXPECT_EQ(text(value), "unknown operator: -Bool");
}

TEST(Evaluator, EvalCompareWithErrorRightIsError) {
  Value value = eval("1 == nope;");

  ASSERT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Error));
  EXPECT_EQ(text(value), "identifier not found: nope");
}

TEST(Evaluator, EvalCompareWithErrorLeftIsError) {
  Value value = eval("nope == 1;");

  ASSERT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Error));
  EXPECT_EQ(text(value), "identifier not found: nope");
}

TEST(Evaluator, EvalLessThanWithErrorIsError) {
  Value value = eval("1 < nope;");

  ASSERT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Error));
  EXPECT_EQ(text(value), "identifier not found: nope");
}

TEST(Evaluator, EvalCompareMissingKeyWithNullIsError) {
  Value value = eval("let h = {}; h[\"a\"] == null;");

  ASSERT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Error));
  EXPECT_EQ(text(value), "key not found: a");
}

TEST(Evaluator, EvalUnaryOnErrorIsError) {
  Value value = eval("-nope;");

  ASSERT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Error));
  EXPECT_EQ(text(value), "identifier not found: nope");
}

TEST(Evaluator, EvalBinaryRightNotEvaluatedAfterErrorLeft) {
  std::string output;
  Value value = evalCapturingOutput("nope + print(\"ran\");", output);

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Error));
  EXPECT_EQ(text(value), "identifier not found: nope");
  EXPECT_EQ(output, "");
}

TEST(Evaluator, EvalModulo) {
  Value value = eval("7 % 2;");

  ASSERT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Number));
  EXPECT_DOUBLE_EQ(value.num, 1.0);
}

TEST(Evaluator, EvalModuloExact) {
  Value value = eval("8 % 4;");

  ASSERT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Number));
  EXPECT_DOUBLE_EQ(value.num, 0.0);
}

TEST(Evaluator, EvalModuloFloat) {
  Value value = eval("7.5 % 2;");

  ASSERT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Number));
  EXPECT_DOUBLE_EQ(value.num, 1.5);
}

TEST(Evaluator, EvalModuloNegative) {
  Value value = eval("-7 % 2;");

  ASSERT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Number));
  EXPECT_DOUBLE_EQ(value.num, -1.0);
}

TEST(Evaluator, EvalModuloBindsTighterThanAdd) {
  Value value = eval("2 + 3 % 2;");

  ASSERT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Number));
  EXPECT_DOUBLE_EQ(value.num, 3.0);
}

TEST(Evaluator, EvalModuloOnStringIsError) {
  Value value = eval("\"a\" % 2;");

  ASSERT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Error));
  EXPECT_EQ(text(value), "unknown operator: Str % Number");
}

TEST(Evaluator, EvalAndBothTrue) {
  Value value = eval("true && true;");

  ASSERT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Bool));
  EXPECT_TRUE(value.boolean);
}

TEST(Evaluator, EvalAndOneFalse) {
  Value value = eval("true && false;");

  ASSERT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Bool));
  EXPECT_FALSE(value.boolean);
}

TEST(Evaluator, EvalOrOneTrue) {
  Value value = eval("false || true;");

  ASSERT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Bool));
  EXPECT_TRUE(value.boolean);
}

TEST(Evaluator, EvalOrBothFalse) {
  Value value = eval("false || false;");

  ASSERT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Bool));
  EXPECT_FALSE(value.boolean);
}

TEST(Evaluator, EvalAndOnComparisons) {
  Value value = eval("1 < 2 && 3 > 2;");

  ASSERT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Bool));
  EXPECT_TRUE(value.boolean);
}

TEST(Evaluator, EvalAndOnNullIsFalse) {
  Value value = eval("null && true;");

  ASSERT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Bool));
  EXPECT_FALSE(value.boolean);
}

TEST(Evaluator, EvalOrOnZeroIsTrue) {
  Value value = eval("0 || false;");

  ASSERT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Bool));
  EXPECT_TRUE(value.boolean);
}

TEST(Evaluator, EvalAndOnStringIsTrue) {
  Value value = eval("\"a\" && \"b\";");

  ASSERT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Bool));
  EXPECT_TRUE(value.boolean);
}

TEST(Evaluator, EvalAndShortCircuitsFalseLeft) {
  std::string output;
  Value value = evalCapturingOutput("false && print(\"ran\");", output);

  ASSERT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Bool));
  EXPECT_FALSE(value.boolean);
  EXPECT_EQ(output, "");
}

TEST(Evaluator, EvalOrShortCircuitsTrueLeft) {
  std::string output;
  Value value = evalCapturingOutput("true || print(\"ran\");", output);

  ASSERT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Bool));
  EXPECT_TRUE(value.boolean);
  EXPECT_EQ(output, "");
}

TEST(Evaluator, EvalAndShortCircuitSkipsError) {
  Value value = eval("false && nope;");

  ASSERT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Bool));
  EXPECT_FALSE(value.boolean);
}

TEST(Evaluator, EvalAndRightErrorPropagates) {
  Value value = eval("true && nope;");

  ASSERT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Error));
  EXPECT_EQ(text(value), "identifier not found: nope");
}

TEST(Evaluator, EvalAndGuardsMissingKey) {
  Value value = eval("let h = {\"a\": 5}; has(h, \"z\") && h[\"z\"] > 1;");

  ASSERT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Bool));
  EXPECT_FALSE(value.boolean);
}

TEST(Evaluator, EvalStringAddConcatenates) {
  Value value = eval("\"foo\" + \"bar\";");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::String));
  EXPECT_EQ(text(value), "foobar");
}

TEST(Evaluator, EvalStringSubtractIsError) {
  Value value = eval("\"foo\" - \"bar\";");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Error));
  EXPECT_EQ(text(value), "unknown operator: Str - Str");
}

TEST(Evaluator, EvalArrayAddConcatenates) {
  Value value = eval("[1, 2] + [3, 4];");

  ASSERT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Array));
  ASSERT_EQ(items(value).size(), 4u);
  EXPECT_DOUBLE_EQ(items(value)[0].num, 1.0);
  EXPECT_DOUBLE_EQ(items(value)[1].num, 2.0);
  EXPECT_DOUBLE_EQ(items(value)[2].num, 3.0);
  EXPECT_DOUBLE_EQ(items(value)[3].num, 4.0);
}

TEST(Evaluator, EvalArrayAddEmptyArrays) {
  Value value = eval("[] + [];");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Array));
  EXPECT_TRUE(items(value).empty());
}

TEST(Evaluator, EvalArrayAddWithEmptyArray) {
  Value value = eval("[1] + [];");

  ASSERT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Array));
  ASSERT_EQ(items(value).size(), 1u);
  EXPECT_DOUBLE_EQ(items(value)[0].num, 1.0);
}

TEST(Evaluator, EvalArrayAddPreservesOrder) {
  Value value = eval("[\"a\", \"b\"] + [\"c\"];");

  ASSERT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Array));
  ASSERT_EQ(items(value).size(), 3u);
  EXPECT_EQ(text(items(value)[0]), "a");
  EXPECT_EQ(text(items(value)[1]), "b");
  EXPECT_EQ(text(items(value)[2]), "c");
}

TEST(Evaluator, EvalArraySubtractIsError) {
  Value value = eval("[1] - [2];");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Error));
  EXPECT_EQ(text(value), "unknown operator: Array - Array");
}

TEST(Evaluator, EvalArrayAddNumberIsError) {
  Value value = eval("[1] + 2;");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Error));
  EXPECT_EQ(text(value), "unknown operator: Array + Number");
}

TEST(Evaluator, EvalHashAddIsError) {
  Value value = eval("{\"a\": 1} + {\"b\": 2};");

  ASSERT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Error));
  EXPECT_EQ(text(value), "unknown operator: HashMap + HashMap");
}

TEST(Evaluator, EvalHashAddNumberIsError) {
  Value value = eval("{\"a\": 1} + 1;");

  ASSERT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Error));
  EXPECT_EQ(text(value), "unknown operator: HashMap + Number");
}

TEST(Evaluator, EvalHashIndexStringKey) {
  Value value = eval("let h = {\"a\": 1}; h[\"a\"];");

  ASSERT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Number));
  EXPECT_DOUBLE_EQ(value.num, 1.0);
}

TEST(Evaluator, EvalHashIndexNumberKey) {
  Value value = eval("let h = {1: \"one\", 2: \"two\"}; h[2];");

  ASSERT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::String));
  EXPECT_EQ(text(value), "two");
}

TEST(Evaluator, EvalHashIndexBoolKey) {
  Value value = eval("let h = {true: \"yes\", false: \"no\"}; h[false];");

  ASSERT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::String));
  EXPECT_EQ(text(value), "no");
}

TEST(Evaluator, EvalHashIndexArrayKeyIsError) {
  Value value = eval("let h = {\"a\": 1}; h[[1]];");

  ASSERT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Error));
  EXPECT_EQ(text(value),
            "hash map key must be a number, string, bool or null: Array");
}

TEST(Evaluator, EvalHashIndexSecondPair) {
  Value value = eval("let h = {\"a\": 1, \"b\": 2}; h[\"b\"];");

  ASSERT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Number));
  EXPECT_DOUBLE_EQ(value.num, 2.0);
}

TEST(Evaluator, EvalHashIndexMissingKeyIsError) {
  Value value = eval("let h = {\"a\": 1}; h[\"b\"];");

  ASSERT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Error));
  EXPECT_EQ(text(value), "key not found: b");
}

TEST(Evaluator, EvalHashIndexWrongKeyKindIsError) {
  Value value = eval("let h = {\"a\": 1}; h[0];");

  ASSERT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Error));
  EXPECT_EQ(text(value), "key not found: 0");
}

TEST(Evaluator, EvalHashIndexNullKey) {
  Value value = eval("let h = {print(): 1}; h[print()];");

  ASSERT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Number));
  EXPECT_DOUBLE_EQ(value.num, 1.0);
}

TEST(Evaluator, EvalHashIndexEmptyHashIsError) {
  Value value = eval("let h = {}; h[\"a\"];");

  ASSERT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Error));
  EXPECT_EQ(text(value), "key not found: a");
}

TEST(Evaluator, EvalHashIndexMissingKeyAfterSetIsError) {
  Value value = eval("let h = {}; set(h, \"a\", 1); h[\"b\"];");

  ASSERT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Error));
  EXPECT_EQ(text(value), "key not found: b");
}

TEST(Evaluator, EvalHashIndexWithBinaryKey) {
  Value value = eval("let h = {2: \"two\"}; h[1 + 1];");

  ASSERT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::String));
  EXPECT_EQ(text(value), "two");
}

TEST(Evaluator, EvalHashIndexNestedHash) {
  Value value = eval("let h = {\"a\": {\"b\": 3}}; h[\"a\"];");

  ASSERT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::HashMap));
  ASSERT_EQ(items(value).size(), 2u);
  EXPECT_DOUBLE_EQ(items(value)[1].num, 3.0);
}

TEST(Evaluator, EvalHashIndexErrorKeyPropagates) {
  Value value = eval("let h = {\"a\": 1}; h[undefinedVar];");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Error));
}

TEST(Evaluator, EvalIndexOnUndefinedVariableIsError) {
  Value value = eval("undefinedVar[0];");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Error));
}

TEST(Evaluator, EvalIndexOnStringIsError) {
  Value value = eval("let s = \"abc\"; s[0];");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Error));
  EXPECT_EQ(text(value), "variable is not an array or a hashmap");
}

TEST(Evaluator, EvalArrayIndexReturnsElement) {
  Value value = eval("let arr = [1, 2, 3]; arr[1];");

  ASSERT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Number));
  EXPECT_DOUBLE_EQ(value.num, 2.0);
}

TEST(Evaluator, EvalArrayIndexFirstElement) {
  Value value = eval("let arr = [\"a\", \"b\"]; arr[0];");

  ASSERT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::String));
  EXPECT_EQ(text(value), "a");
}

TEST(Evaluator, EvalArrayIndexOutOfBoundsIsError) {
  Value value = eval("let arr = [1, 2]; arr[2];");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Error));
  EXPECT_EQ(text(value), "index is bigger than array size");
}

TEST(Evaluator, EvalArrayIndexNegativeIsError) {
  Value value = eval("let arr = [1, 2]; arr[-1];");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Error));
  EXPECT_EQ(text(value), "index must be greater or equal than zero");
}

TEST(Evaluator, EvalArrayIndexOnNonArrayIsError) {
  Value value = eval("let notArr = 5; notArr[0];");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Error));
  EXPECT_EQ(text(value), "variable is not an array or a hashmap");
}

TEST(Evaluator, EvalArrayIndexNonNumberIsError) {
  Value value = eval("let arr = [1, 2]; arr[true];");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Error));
  EXPECT_EQ(text(value), "index must be a number");
}

TEST(Evaluator, EvalArrayIndexWithBinaryExpression) {
  Value value = eval("let arr = [1, 2, 3]; arr[1 + 1];");

  ASSERT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Number));
  EXPECT_DOUBLE_EQ(value.num, 3.0);
}

TEST(Evaluator, EvalArrayIndexWithVariable) {
  Value value = eval("let arr = [1, 2, 3]; let i = 2; arr[i];");

  ASSERT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Number));
  EXPECT_DOUBLE_EQ(value.num, 3.0);
}

TEST(Evaluator, EvalErrorStopsFollowingStatements) {
  Value value = eval("-true; 5;");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Error));
  EXPECT_EQ(text(value), "unknown operator: -Bool");
}

TEST(Evaluator, EvalErrorInsideIfBlockStopsFollowingStatements) {
  Value value = eval("if (true) { -true; 9; }");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Error));
  EXPECT_EQ(text(value), "unknown operator: -Bool");
}

TEST(Evaluator, EvalErrorConditionSkipsIfBranches) {
  Value value = eval("if (-true) { 9; } else { 10; }");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Error));
  EXPECT_EQ(text(value), "unknown operator: -Bool");
}

TEST(Evaluator, EvalIfTrueConditionRunsConsequence) {
  Value value = eval("if (true) { 1; } else { 2; }");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Number));
  EXPECT_DOUBLE_EQ(value.num, 1.0);
}

TEST(Evaluator, EvalIfFalseConditionRunsAlternative) {
  Value value = eval("if (false) { 1; } else { 2; }");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Number));
  EXPECT_DOUBLE_EQ(value.num, 2.0);
}

TEST(Evaluator, EvalIfWithoutElseFalseConditionIsNull) {
  Value value = eval("if (false) { 1; }");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Null));
}

TEST(Evaluator, EvalIfWithoutElseTrueCondition) {
  Value value = eval("if (true) { 1; }");

  EXPECT_DOUBLE_EQ(value.num, 1.0);
}

TEST(Evaluator, EvalIfNumberConditionIsTruthy) {
  Value value = eval("if (0) { 1; } else { 2; }");

  EXPECT_DOUBLE_EQ(value.num, 1.0);
}

TEST(Evaluator, EvalIfStringConditionIsTruthy) {
  Value value = eval("if (\"\") { 1; } else { 2; }");

  EXPECT_DOUBLE_EQ(value.num, 1.0);
}

TEST(Evaluator, EvalIfEmptyArrayConditionIsTruthy) {
  Value value = eval("if ([]) { 1; } else { 2; }");

  EXPECT_DOUBLE_EQ(value.num, 1.0);
}

TEST(Evaluator, EvalIfComparisonConditionTrue) {
  Value value = eval("if (2 > 1) { 1; } else { 2; }");

  EXPECT_DOUBLE_EQ(value.num, 1.0);
}

TEST(Evaluator, EvalIfComparisonConditionFalse) {
  Value value = eval("if (1 > 2) { 1; } else { 2; }");

  EXPECT_DOUBLE_EQ(value.num, 2.0);
}

TEST(Evaluator, EvalIfNegatedConditionRunsAlternative) {
  Value value = eval("if (!true) { 1; } else { 2; }");

  EXPECT_DOUBLE_EQ(value.num, 2.0);
}

TEST(Evaluator, EvalIfConsequenceReturnsLastStatement) {
  Value value = eval("if (true) { 1; 2; 3; } else { 9; }");

  EXPECT_DOUBLE_EQ(value.num, 3.0);
}

TEST(Evaluator, EvalIfAlternativeReturnsLastStatement) {
  Value value = eval("if (false) { 1; } else { 8; 9; }");

  EXPECT_DOUBLE_EQ(value.num, 9.0);
}

TEST(Evaluator, EvalIfEmptyConsequenceIsNull) {
  Value value = eval("if (true) { }");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Null));
}

TEST(Evaluator, EvalIfEmptyAlternativeIsNull) {
  Value value = eval("if (false) { 1; } else { }");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Null));
}

TEST(Evaluator, EvalIfElseStringResult) {
  Value value = eval("if (false) { \"a\"; } else { \"b\"; }");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::String));
  EXPECT_EQ(text(value), "b");
}

TEST(Evaluator, EvalIfElseBooleanResult) {
  Value value = eval("if (false) { true; } else { false; }");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Bool));
  EXPECT_FALSE(value.boolean);
}

TEST(Evaluator, EvalIfElseWithVariableCondition) {
  Value value = eval("let x = 5; if (x > 3) { 10; } else { 20; }");

  EXPECT_DOUBLE_EQ(value.num, 10.0);
}

TEST(Evaluator, EvalIfElseAlternativeWithVariableCondition) {
  Value value = eval("let x = 1; if (x > 3) { 10; } else { 20; }");

  EXPECT_DOUBLE_EQ(value.num, 20.0);
}

TEST(Evaluator, EvalIfElseVarBindingInAlternativeIsVisibleOutside) {
  Value value = eval("if (false) { let x = 1; } else { let x = 2; } x;");

  EXPECT_DOUBLE_EQ(value.num, 2.0);
}

TEST(Evaluator, EvalIfElseErrorInAlternativeIsReturned) {
  Value value = eval("if (false) { 1; } else { -true; }");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Error));
  EXPECT_EQ(text(value), "unknown operator: -Bool");
}

TEST(Evaluator, EvalIfElseUnboundConditionIsError) {
  Value value = eval("if (nope) { 1; } else { 2; }");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Error));
  EXPECT_EQ(text(value), "identifier not found: nope");
}

TEST(Evaluator, EvalIfElseReturnInsideAlternative) {
  Value value = eval("if (false) { 1; } else { return 5; 6; }");

  EXPECT_DOUBLE_EQ(value.num, 5.0);
}

TEST(Evaluator, EvalIfElseFollowedByStatement) {
  Value value = eval("if (true) { 1; } else { 2; } 3;");

  EXPECT_DOUBLE_EQ(value.num, 3.0);
}

TEST(Evaluator, EvalNestedIfElseInConsequence) {
  Value value = eval("if (true) { if (false) { 1; } else { 2; } } else { 3; }");

  EXPECT_DOUBLE_EQ(value.num, 2.0);
}

TEST(Evaluator, EvalNestedIfElseInAlternative) {
  Value value = eval("if (false) { 1; } else { if (true) { 2; } else { 3; } }");

  EXPECT_DOUBLE_EQ(value.num, 2.0);
}

TEST(Evaluator, EvalIfElseInsideFunctionBody) {
  Value value = eval("let f = fn(x) { if (x > 0) { 1; } else { 0; } }; f(5);");

  EXPECT_DOUBLE_EQ(value.num, 1.0);
}

TEST(Evaluator, EvalIfElseInsideFunctionBodyAlternative) {
  Value value = eval("let f = fn(x) { if (x > 0) { 1; } else { 0; } }; f(-5);");

  EXPECT_DOUBLE_EQ(value.num, 0.0);
}

TEST(Evaluator, EvalIfElseIfFirstBranch) {
  Value value = eval("if (true) { 1; } else if (true) { 2; } else { 3; }");

  EXPECT_DOUBLE_EQ(value.num, 1.0);
}

TEST(Evaluator, EvalIfElseIfSecondBranch) {
  Value value = eval("if (false) { 1; } else if (true) { 2; } else { 3; }");

  EXPECT_DOUBLE_EQ(value.num, 2.0);
}

TEST(Evaluator, EvalIfElseIfFinalElse) {
  Value value = eval("if (false) { 1; } else if (false) { 2; } else { 3; }");

  EXPECT_DOUBLE_EQ(value.num, 3.0);
}

TEST(Evaluator, EvalIfElseIfWithoutFinalElseIsNull) {
  Value value = eval("if (false) { 1; } else if (false) { 2; }");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Null));
}

TEST(Evaluator, EvalIfElseIfChainThirdBranch) {
  Value value = eval("if (false) { 1; } else if (false) { 2; } else if (true) "
                     "{ 3; } else { 4; }");

  EXPECT_DOUBLE_EQ(value.num, 3.0);
}

TEST(Evaluator, EvalIfElseIfChainComparisons) {
  Value value = eval("let x = 5; if (x < 0) { \"neg\"; } else if (x == 0) { "
                     "\"zero\"; } else { \"pos\"; }");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::String));
  EXPECT_EQ(text(value), "pos");
}

TEST(Evaluator, EvalIfElseIfErrorConditionIsReturned) {
  Value value = eval("if (false) { 1; } else if (-true) { 2; } else { 3; }");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Error));
  EXPECT_EQ(text(value), "unknown operator: -Bool");
}

TEST(Evaluator, EvalIfElseIfBranchMultipleStatements) {
  Value value = eval("if (false) { 1; } else if (true) { 2; 3; } else { 4; }");

  EXPECT_DOUBLE_EQ(value.num, 3.0);
}

TEST(Evaluator, EvalVarStatementReturnsInitializerValue) {
  Value value = eval("let x = 5;");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Number));
  EXPECT_DOUBLE_EQ(value.num, 5.0);
}

TEST(Evaluator, EvalVarStatementBindsName) {
  Value value = eval("let x = 5; x;");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Number));
  EXPECT_DOUBLE_EQ(value.num, 5.0);
}

TEST(Evaluator, EvalVarStatementBindsString) {
  Value value = eval("let greeting = \"hello\"; greeting;");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::String));
  EXPECT_EQ(text(value), "hello");
}

TEST(Evaluator, EvalVarStatementBindsBoolean) {
  Value value = eval("let flag = false; flag;");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Bool));
  EXPECT_FALSE(value.boolean);
}

TEST(Evaluator, EvalVarStatementEvaluatesInitializer) {
  Value value = eval("let x = 2 * 3 + 4; x;");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Number));
  EXPECT_DOUBLE_EQ(value.num, 10.0);
}

TEST(Evaluator, EvalVarStatementInitializerSeesEarlierNames) {
  Value value = eval("let a = 1; let b = a + 2; let c = a + b; c;");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Number));
  EXPECT_DOUBLE_EQ(value.num, 4.0);
}

TEST(Evaluator, EvalVarStatementRebindingOverwrites) {
  Value value = eval("let x = 5; let x = 9; x;");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Number));
  EXPECT_DOUBLE_EQ(value.num, 9.0);
}

TEST(Evaluator, EvalVarStatementRebindingChangesKind) {
  Value value = eval("let x = 5; let x = \"five\"; x;");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::String));
  EXPECT_EQ(text(value), "five");
}

TEST(Evaluator, EvalVarStatementNameIsUsableInExpressions) {
  Value value = eval("let x = 5; x * 2;");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Number));
  EXPECT_DOUBLE_EQ(value.num, 10.0);
}

TEST(Evaluator, EvalVarStatementNameIsUsableAsIfCondition) {
  Value value = eval("let flag = false; if (flag) { 1; } else { 2; }");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Number));
  EXPECT_DOUBLE_EQ(value.num, 2.0);
}

TEST(Evaluator, EvalVarStatementWithoutInitializerIsNull) {
  Value value = eval("let x;");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Null));
}

TEST(Evaluator, EvalVarStatementWithoutInitializerBindsNull) {
  Value value = eval("let x; x;");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Null));
}

TEST(Evaluator, EvalVarStatementErrorInitializerStopsProgram) {
  Value value = eval("let x = -true; 5;");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Error));
  EXPECT_EQ(text(value), "unknown operator: -Bool");
}

TEST(Evaluator, EvalVarStatementInsideBlockIsVisibleOutside) {
  Value value = eval("if (true) { let x = 7; } x;");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Number));
  EXPECT_DOUBLE_EQ(value.num, 7.0);
}

TEST(Evaluator, EvalUnboundNameIsError) {
  Value value = eval("nope;");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Error));
  EXPECT_EQ(text(value), "identifier not found: nope");
}

TEST(Evaluator, EvalCallExpressionAddsArgs) {
  Value value = eval("let add = fn(x, y) { x + y; }; add(1, 2);");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Number));
  EXPECT_DOUBLE_EQ(value.num, 3.0);
}

TEST(Evaluator, EvalCallExpressionMultipleBodyStatementsReturnsLast) {
  Value value = eval("let f = fn(x) { let y = x + 1; y * 2; }; f(3);");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Number));
  EXPECT_DOUBLE_EQ(value.num, 8.0);
}

TEST(Evaluator, EvalCallExpressionArgumentsAreLexicallyScoped) {
  Value value = eval("let x = 10; let f = fn(x) { x; }; f(5); x;");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Number));
  EXPECT_DOUBLE_EQ(value.num, 10.0);
}

TEST(Evaluator, EvalCallExpressionReturnStopsBodyEarly) {
  Value value = eval("let f = fn(x) { return x + 1; x + 100; }; f(2);");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Number));
  EXPECT_DOUBLE_EQ(value.num, 3.0);
}

TEST(Evaluator, EvalCallExpressionArgumentCountMismatchIsError) {
  Value value = eval("let add = fn(x, y) { x + y; }; add(1);");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Error));
  EXPECT_EQ(text(value), "wrong number of arguments: got 1, want 2");
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
  EXPECT_EQ(evalOutput("let x; print(x);"), "null\n");
}

TEST(Evaluator, EvalBuiltinPrintWithoutArgs) {
  EXPECT_EQ(evalOutput("print();"), "\n");
}

TEST(Evaluator, EvalBuiltinPrintSeparatesArgsWithSpace) {
  EXPECT_EQ(evalOutput("print(\"a\", 1, false);"), "a 1 false\n");
}

TEST(Evaluator, EvalBuiltinPrintEvaluatesArgs) {
  EXPECT_EQ(evalOutput("let name = \"ayga\"; print(\"hi \" + name);"),
            "hi ayga\n");
}

TEST(Evaluator, EvalBuiltinPrintEveryCallOutputs) {
  EXPECT_EQ(evalOutput("print(\"a\"); print(\"b\");"), "a\nb\n");
}

TEST(Evaluator, EvalBuiltinPrintInsideFunctionBody) {
  EXPECT_EQ(evalOutput("let f = fn(x) { print(x); }; f(7);"), "7\n");
}

TEST(Evaluator, EvalBuiltinPrintReturnsNull) {
  std::string output;
  Value value = evalCapturingOutput("print(\"hello\");", output);

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Null));
}

TEST(Evaluator, EvalBuiltinPrintArgumentErrorIsReturned) {
  std::string output;
  Value value = evalCapturingOutput("print(-true);", output);

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Error));
  EXPECT_EQ(text(value), "unknown operator: -Bool");
  EXPECT_EQ(output, "");
}

TEST(Evaluator, EvalBuiltinPrintUnboundArgumentIsError) {
  std::string output;
  Value value = evalCapturingOutput("print(nope);", output);

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Error));
  EXPECT_EQ(text(value), "identifier not found: nope");
  EXPECT_EQ(output, "");
}

TEST(Evaluator, EvalBuiltinPrintIsNotShadowedByVar) {
  EXPECT_EQ(evalOutput("let print = 5; print(\"hello\");"), "hello\n");
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

TEST(Evaluator, EvalBuiltinPrintEmptyHash) {
  EXPECT_EQ(evalOutput("print({});"), "{}\n");
}

TEST(Evaluator, EvalBuiltinPrintHashSinglePair) {
  EXPECT_EQ(evalOutput("print({\"one\": 1});"), "{one:1}\n");
}

TEST(Evaluator, EvalBuiltinPrintHashMultiplePairs) {
  EXPECT_EQ(evalOutput("print({\"one\": 1, \"two\": 2, \"three\": 3});"),
            "{one:1,two:2,three:3}\n");
}

TEST(Evaluator, EvalBuiltinPrintHashBoolValue) {
  EXPECT_EQ(evalOutput("print({\"ok\": true});"), "{ok:true}\n");
}

TEST(Evaluator, EvalBuiltinPrintHashArrayValue) {
  EXPECT_EQ(evalOutput("print({\"a\": [1, 2]});"), "{a:[1,2]}\n");
}

TEST(Evaluator, EvalBuiltinPrintNestedHash) {
  EXPECT_EQ(evalOutput("print({\"a\": {\"b\": 1}});"), "{a:{b:1}}\n");
}

TEST(Evaluator, EvalBuiltinPrintArrayOfHashes) {
  EXPECT_EQ(evalOutput("print([{\"a\": 1}]);"), "[{a:1}]\n");
}

TEST(Evaluator, EvalBuiltinLenHash) {
  Value value = eval("len({\"a\": 1});");

  ASSERT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Number));
  EXPECT_DOUBLE_EQ(value.num, 1.0);
}

TEST(Evaluator, EvalBuiltinLenHashMultiplePairs) {
  Value value = eval("len({\"a\": 1, \"b\": 2, \"c\": 3});");

  ASSERT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Number));
  EXPECT_DOUBLE_EQ(value.num, 3.0);
}

TEST(Evaluator, EvalBuiltinLenEmptyHash) {
  Value value = eval("len({});");

  ASSERT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Number));
  EXPECT_DOUBLE_EQ(value.num, 0.0);
}

TEST(Evaluator, EvalBuiltinPushHashIsError) {
  Value value = eval("let h = {}; push(h, 1);");

  ASSERT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Error));
  EXPECT_EQ(text(value), "argument to push is not an array: HashMap");
}

TEST(Evaluator, EvalBuiltinLenString) {
  Value value = eval("len(\"hello\");");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Number));
  EXPECT_DOUBLE_EQ(value.num, 5.0);
}

TEST(Evaluator, EvalBuiltinLenEmptyString) {
  Value value = eval("len(\"\");");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Number));
  EXPECT_DOUBLE_EQ(value.num, 0.0);
}

TEST(Evaluator, EvalBuiltinLenEvaluatesArgs) {
  Value value = eval("let name = \"ayga\"; len(\"hi \" + name);");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Number));
  EXPECT_DOUBLE_EQ(value.num, 7.0);
}

TEST(Evaluator, EvalBuiltinLenIsNotShadowedByVar) {
  Value value = eval("let len = 5; len(\"abc\");");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Number));
  EXPECT_DOUBLE_EQ(value.num, 3.0);
}

TEST(Evaluator, EvalBuiltinLenWithoutArgsIsError) {
  std::string output;
  Value value = evalCapturingOutput("len();", output);

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Error));
  EXPECT_EQ(text(value), "wrong number of arguments: got 0, want 1");
  EXPECT_EQ(output, "");
}

TEST(Evaluator, EvalBuiltinLenTooManyArgsIsError) {
  std::string output;
  Value value = evalCapturingOutput("len(\"a\", \"b\");", output);

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Error));
  EXPECT_EQ(text(value), "wrong number of arguments: got 2, want 1");
  EXPECT_EQ(output, "");
}

TEST(Evaluator, EvalBuiltinLenNumberArgumentIsError) {
  std::string output;
  Value value = evalCapturingOutput("len(1);", output);

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Error));
  EXPECT_EQ(text(value), "argument to len is not supported: Number");
  EXPECT_EQ(output, "");
}

TEST(Evaluator, EvalBuiltinLenBoolArgumentIsError) {
  std::string output;
  Value value = evalCapturingOutput("len(true);", output);

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Error));
  EXPECT_EQ(text(value), "argument to len is not supported: Bool");
  EXPECT_EQ(output, "");
}

TEST(Evaluator, EvalBuiltinLenNullArgumentIsError) {
  std::string output;
  Value value = evalCapturingOutput("let x; len(x);", output);

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Error));
  EXPECT_EQ(text(value), "argument to len is not supported: Null");
  EXPECT_EQ(output, "");
}

TEST(Evaluator, EvalBuiltinLenArgumentErrorIsReturned) {
  std::string output;
  Value value = evalCapturingOutput("len(-true);", output);

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Error));
  EXPECT_EQ(text(value), "unknown operator: -Bool");
  EXPECT_EQ(output, "");
}

TEST(Evaluator, EvalBuiltinLenUnboundArgumentIsError) {
  std::string output;
  Value value = evalCapturingOutput("len(nope);", output);

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Error));
  EXPECT_EQ(text(value), "identifier not found: nope");
  EXPECT_EQ(output, "");
}

TEST(Evaluator, EvalBuiltinPushReturnsPushedValue) {
  Value value = eval("let arr = [1]; push(arr, 2);");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Number));
  EXPECT_DOUBLE_EQ(value.num, 2.0);
}

TEST(Evaluator, EvalBuiltinPushAppendsToArray) {
  Value value = eval("let arr = [1, 2]; push(arr, 3); arr;");

  ASSERT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Array));
  ASSERT_EQ(items(value).size(), 3u);
  EXPECT_DOUBLE_EQ(items(value)[0].num, 1.0);
  EXPECT_DOUBLE_EQ(items(value)[1].num, 2.0);
  EXPECT_DOUBLE_EQ(items(value)[2].num, 3.0);
}

TEST(Evaluator, EvalBuiltinPushOnEmptyArray) {
  Value value = eval("let arr = []; push(arr, 1); arr;");

  ASSERT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Array));
  ASSERT_EQ(items(value).size(), 1u);
  EXPECT_DOUBLE_EQ(items(value)[0].num, 1.0);
}

TEST(Evaluator, EvalBuiltinPushEvaluatesSecondArgument) {
  Value value = eval("let arr = []; push(arr, 1 + 2); arr;");

  ASSERT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Array));
  ASSERT_EQ(items(value).size(), 1u);
  EXPECT_DOUBLE_EQ(items(value)[0].num, 3.0);
}

TEST(Evaluator, EvalBuiltinPushTooFewArgsIsError) {
  std::string output;
  Value value = evalCapturingOutput("push(1);", output);

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Error));
  EXPECT_EQ(text(value), "wrong number of arguments: got 1, want 2");
  EXPECT_EQ(output, "");
}

TEST(Evaluator, EvalBuiltinPushTooManyArgsIsError) {
  std::string output;
  Value value = evalCapturingOutput("push(1, 2, 3);", output);

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Error));
  EXPECT_EQ(text(value), "wrong number of arguments: got 3, want 2");
  EXPECT_EQ(output, "");
}

TEST(Evaluator, EvalBuiltinPushFirstArgumentNotIdentifierIsError) {
  std::string output;
  Value value = evalCapturingOutput("push([1, 2], 3);", output);

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Error));
  EXPECT_EQ(text(value), "first argument to push must be an identifier");
  EXPECT_EQ(output, "");
}

TEST(Evaluator, EvalBuiltinPushOnNonArrayIsError) {
  std::string output;
  Value value = evalCapturingOutput("let notArr = 5; push(notArr, 1);", output);

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Error));
  EXPECT_EQ(text(value), "argument to push is not an array: Number");
  EXPECT_EQ(output, "");
}

TEST(Evaluator, EvalBuiltinPushUnboundArrayIsError) {
  std::string output;
  Value value = evalCapturingOutput("push(nope, 1);", output);

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Error));
  EXPECT_EQ(text(value), "identifier not found: nope");
  EXPECT_EQ(output, "");
}

TEST(Evaluator, EvalBuiltinPushSecondArgumentErrorIsReturned) {
  std::string output;
  Value value = evalCapturingOutput("let arr = []; push(arr, -true);", output);

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Error));
  EXPECT_EQ(text(value), "unknown operator: -Bool");
  EXPECT_EQ(output, "");
}

TEST(Evaluator, EvalBuiltinSetReturnsSetValue) {
  Value value = eval("let h = {}; set(h, \"a\", 1);");

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Number));
  EXPECT_DOUBLE_EQ(value.num, 1.0);
}

TEST(Evaluator, EvalBuiltinSetAddsPairToEmptyHash) {
  Value value = eval("let h = {}; set(h, \"a\", 1); h;");

  ASSERT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::HashMap));
  ASSERT_EQ(items(value).size(), 2u);
  EXPECT_EQ(text(items(value)[0]), "a");
  EXPECT_DOUBLE_EQ(items(value)[1].num, 1.0);
}

TEST(Evaluator, EvalBuiltinSetAppendsNewKey) {
  Value value = eval("let h = {\"a\": 1}; set(h, \"b\", 2); h;");

  ASSERT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::HashMap));
  ASSERT_EQ(items(value).size(), 4u);
  EXPECT_EQ(text(items(value)[0]), "a");
  EXPECT_DOUBLE_EQ(items(value)[1].num, 1.0);
  EXPECT_EQ(text(items(value)[2]), "b");
  EXPECT_DOUBLE_EQ(items(value)[3].num, 2.0);
}

TEST(Evaluator, EvalBuiltinSetReplacesExistingKey) {
  Value value = eval("let h = {\"a\": 1, \"b\": 2}; set(h, \"a\", 9); h;");

  ASSERT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::HashMap));
  ASSERT_EQ(items(value).size(), 4u);
  EXPECT_EQ(text(items(value)[0]), "a");
  EXPECT_DOUBLE_EQ(items(value)[1].num, 9.0);
  EXPECT_EQ(text(items(value)[2]), "b");
  EXPECT_DOUBLE_EQ(items(value)[3].num, 2.0);
}

TEST(Evaluator, EvalBuiltinSetReplacingKeyKeepsLen) {
  Value value = eval("let h = {\"a\": 1}; set(h, \"a\", 2); len(h);");

  ASSERT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Number));
  EXPECT_DOUBLE_EQ(value.num, 1.0);
}

TEST(Evaluator, EvalBuiltinSetNewKeyGrowsLen) {
  Value value = eval("let h = {\"a\": 1}; set(h, \"b\", 2); len(h);");

  ASSERT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Number));
  EXPECT_DOUBLE_EQ(value.num, 2.0);
}

TEST(Evaluator, EvalBuiltinSetValueIsReadableByIndex) {
  Value value = eval("let h = {}; set(h, \"a\", 1); h[\"a\"];");

  ASSERT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Number));
  EXPECT_DOUBLE_EQ(value.num, 1.0);
}

TEST(Evaluator, EvalBuiltinSetReplacedValueIsReadableByIndex) {
  Value value = eval("let h = {\"a\": 1}; set(h, \"a\", 9); h[\"a\"];");

  ASSERT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Number));
  EXPECT_DOUBLE_EQ(value.num, 9.0);
}

TEST(Evaluator, EvalBuiltinSetEvaluatesKeyArgument) {
  Value value = eval("let h = {}; set(h, \"a\" + \"b\", 1); h[\"ab\"];");

  ASSERT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Number));
  EXPECT_DOUBLE_EQ(value.num, 1.0);
}

TEST(Evaluator, EvalBuiltinSetEvaluatesValueArgument) {
  Value value = eval("let h = {}; set(h, \"a\", 1 + 2); h[\"a\"];");

  ASSERT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Number));
  EXPECT_DOUBLE_EQ(value.num, 3.0);
}

TEST(Evaluator, EvalBuiltinSetHashValue) {
  Value value = eval("let h = {}; set(h, \"a\", {\"b\": 1}); h;");

  ASSERT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::HashMap));
  ASSERT_EQ(items(value).size(), 2u);
  EXPECT_EQ(static_cast<int>(kindOf(items(value)[1])),
            static_cast<int>(ValueKind::HashMap));
}

TEST(Evaluator, EvalBuiltinSetTwiceAddsBothKeys) {
  Value value =
      eval("let h = {}; set(h, \"a\", 1); set(h, \"b\", 2); len(h);");

  ASSERT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Number));
  EXPECT_DOUBLE_EQ(value.num, 2.0);
}

TEST(Evaluator, EvalBuiltinSetTooFewArgsIsError) {
  std::string output;
  Value value = evalCapturingOutput("set(1, 2);", output);

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Error));
  EXPECT_EQ(text(value), "wrong number of arguments: got 2, want 3");
  EXPECT_EQ(output, "");
}

TEST(Evaluator, EvalBuiltinSetTooManyArgsIsError) {
  std::string output;
  Value value = evalCapturingOutput("set(1, 2, 3, 4);", output);

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Error));
  EXPECT_EQ(text(value), "wrong number of arguments: got 4, want 3");
  EXPECT_EQ(output, "");
}

TEST(Evaluator, EvalBuiltinSetFirstArgumentNotIdentifierIsError) {
  std::string output;
  Value value = evalCapturingOutput("set({}, \"a\", 1);", output);

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Error));
  EXPECT_EQ(text(value), "first argument to set must be an identifier");
  EXPECT_EQ(output, "");
}

TEST(Evaluator, EvalBuiltinSetOnNonHashMapIsError) {
  std::string output;
  Value value =
      evalCapturingOutput("let notMap = 5; set(notMap, \"a\", 1);", output);

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Error));
  EXPECT_EQ(text(value), "argument to set is not a hashMap: Number");
  EXPECT_EQ(output, "");
}

TEST(Evaluator, EvalBuiltinSetOnArrayIsError) {
  std::string output;
  Value value =
      evalCapturingOutput("let arr = [1]; set(arr, \"a\", 1);", output);

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Error));
  EXPECT_EQ(text(value), "argument to set is not a hashMap: Array");
  EXPECT_EQ(output, "");
}

TEST(Evaluator, EvalBuiltinSetUnboundHashMapIsError) {
  std::string output;
  Value value = evalCapturingOutput("set(nope, \"a\", 1);", output);

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Error));
  EXPECT_EQ(text(value), "identifier not found: nope");
  EXPECT_EQ(output, "");
}

TEST(Evaluator, EvalBuiltinSetNumberKey) {
  Value value = eval("let h = {}; set(h, 1, 2); h[1];");

  ASSERT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Number));
  EXPECT_DOUBLE_EQ(value.num, 2.0);
}

TEST(Evaluator, EvalBuiltinSetBoolKey) {
  Value value = eval("let h = {}; set(h, true, 2); h[true];");

  ASSERT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Number));
  EXPECT_DOUBLE_EQ(value.num, 2.0);
}

TEST(Evaluator, EvalBuiltinSetReplacesNumberKey) {
  Value value = eval("let h = {1: \"one\"}; set(h, 1, \"uno\"); len(h);");

  ASSERT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Number));
  EXPECT_DOUBLE_EQ(value.num, 1.0);
}

TEST(Evaluator, EvalBuiltinSetArrayKeyIsError) {
  std::string output;
  Value value = evalCapturingOutput("let h = {}; set(h, [1], 2);", output);

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Error));
  EXPECT_EQ(text(value),
            "hash map key must be a number, string, bool or null: Array");
  EXPECT_EQ(output, "");
}

TEST(Evaluator, EvalBuiltinSetHashKeyIsError) {
  std::string output;
  Value value =
      evalCapturingOutput("let h = {}; set(h, {\"a\": 1}, 2);", output);

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Error));
  EXPECT_EQ(text(value),
            "hash map key must be a number, string, bool or null: HashMap");
  EXPECT_EQ(output, "");
}

TEST(Evaluator, EvalBuiltinSetKeyErrorIsReturned) {
  std::string output;
  Value value = evalCapturingOutput("let h = {}; set(h, -true, 1);", output);

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Error));
  EXPECT_EQ(text(value), "unknown operator: -Bool");
  EXPECT_EQ(output, "");
}

TEST(Evaluator, EvalBuiltinSetValueErrorIsReturned) {
  std::string output;
  Value value = evalCapturingOutput("let h = {}; set(h, \"a\", -true);", output);

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Error));
  EXPECT_EQ(text(value), "unknown operator: -Bool");
  EXPECT_EQ(output, "");
}

TEST(Evaluator, EvalBuiltinHasExistingKey) {
  Value value = eval("let h = {\"a\": 1}; has(h, \"a\");");

  ASSERT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Bool));
  EXPECT_TRUE(value.boolean);
}

TEST(Evaluator, EvalBuiltinHasMissingKey) {
  Value value = eval("let h = {\"a\": 1}; has(h, \"b\");");

  ASSERT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Bool));
  EXPECT_FALSE(value.boolean);
}

TEST(Evaluator, EvalBuiltinHasEmptyHash) {
  Value value = eval("let h = {}; has(h, \"a\");");

  ASSERT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Bool));
  EXPECT_FALSE(value.boolean);
}

TEST(Evaluator, EvalBuiltinHasNumberKey) {
  Value value = eval("let h = {2: \"two\"}; has(h, 2);");

  ASSERT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Bool));
  EXPECT_TRUE(value.boolean);
}

TEST(Evaluator, EvalBuiltinHasMissingNumberKey) {
  Value value = eval("let h = {2: \"two\"}; has(h, 3);");

  ASSERT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Bool));
  EXPECT_FALSE(value.boolean);
}

TEST(Evaluator, EvalBuiltinHasKeyAddedBySet) {
  Value value = eval("let h = {}; set(h, \"a\", 1); has(h, \"a\");");

  ASSERT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Bool));
  EXPECT_TRUE(value.boolean);
}

TEST(Evaluator, EvalBuiltinHasEvaluatesKeyArgument) {
  Value value = eval("let h = {\"ab\": 1}; has(h, \"a\" + \"b\");");

  ASSERT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Bool));
  EXPECT_TRUE(value.boolean);
}

TEST(Evaluator, EvalBuiltinHasArrayKeyIsError) {
  std::string output;
  Value value = evalCapturingOutput("let h = {}; has(h, [1]);", output);

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Error));
  EXPECT_EQ(text(value),
            "hash map key must be a number, string, bool or null: Array");
  EXPECT_EQ(output, "");
}

TEST(Evaluator, EvalBuiltinHasGuardsIndexing) {
  Value value = eval("let h = {\"a\": 1}; if (has(h, \"a\")) { h[\"a\"]; }");

  ASSERT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Number));
  EXPECT_DOUBLE_EQ(value.num, 1.0);
}

TEST(Evaluator, EvalBuiltinHasTooFewArgsIsError) {
  std::string output;
  Value value = evalCapturingOutput("has(1);", output);

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Error));
  EXPECT_EQ(text(value), "wrong number of arguments: got 1, want 2");
  EXPECT_EQ(output, "");
}

TEST(Evaluator, EvalBuiltinHasTooManyArgsIsError) {
  std::string output;
  Value value = evalCapturingOutput("has(1, 2, 3);", output);

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Error));
  EXPECT_EQ(text(value), "wrong number of arguments: got 3, want 2");
  EXPECT_EQ(output, "");
}

TEST(Evaluator, EvalBuiltinHasFirstArgumentNotIdentifierIsError) {
  std::string output;
  Value value = evalCapturingOutput("has({\"a\": 1}, \"a\");", output);

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Error));
  EXPECT_EQ(text(value), "first argument to has must be an identifier");
  EXPECT_EQ(output, "");
}

TEST(Evaluator, EvalBuiltinHasOnNonHashMapIsError) {
  std::string output;
  Value value =
      evalCapturingOutput("let notMap = 5; has(notMap, \"a\");", output);

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Error));
  EXPECT_EQ(text(value), "argument to has is not a hashMap: Number");
  EXPECT_EQ(output, "");
}

TEST(Evaluator, EvalBuiltinHasOnArrayIsError) {
  std::string output;
  Value value = evalCapturingOutput("let arr = [1]; has(arr, \"a\");", output);

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Error));
  EXPECT_EQ(text(value), "argument to has is not a hashMap: Array");
  EXPECT_EQ(output, "");
}

TEST(Evaluator, EvalBuiltinHasUnboundHashMapIsError) {
  std::string output;
  Value value = evalCapturingOutput("has(nope, \"a\");", output);

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Error));
  EXPECT_EQ(text(value), "identifier not found: nope");
  EXPECT_EQ(output, "");
}

TEST(Evaluator, EvalBuiltinHasKeyErrorIsReturned) {
  std::string output;
  Value value = evalCapturingOutput("let h = {}; has(h, -true);", output);

  EXPECT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Error));
  EXPECT_EQ(text(value), "unknown operator: -Bool");
  EXPECT_EQ(output, "");
}

TEST(Evaluator, EvalHashLiteralDuplicateKeyKeepsLastValue) {
  Value value = eval("{\"a\": 1, \"a\": 2};");

  ASSERT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::HashMap));
  ASSERT_EQ(items(value).size(), 2u);
  EXPECT_EQ(text(items(value)[0]), "a");
  EXPECT_DOUBLE_EQ(items(value)[1].num, 2.0);
}

TEST(Evaluator, EvalHashLiteralDuplicateKeyLen) {
  Value value = eval("len({\"a\": 1, \"b\": 2, \"a\": 3});");

  ASSERT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::Number));
  EXPECT_DOUBLE_EQ(value.num, 2.0);
}

TEST(Evaluator, EvalHashLiteralDuplicateNumberKeyKeepsLastValue) {
  Value value = eval("let h = {1: \"one\", 1: \"uno\"}; h[1];");

  ASSERT_EQ(static_cast<int>(kindOf(value)), static_cast<int>(ValueKind::String));
  EXPECT_EQ(text(value), "uno");
}

} // namespace
