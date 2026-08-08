#pragma once
#include "environment.h"
#include "parser/expression.h"
#include "parser/parser.h"
#include "value.h"

struct Evaluator {
  ParserResult parserResult;
  Environment environment = Environment{};
  // Returns the value of the last statement.
  Value evalStatements();
  Value evalStatement(int index);
  Value evalExpression(int index);
  Value evalPrefixExpression(UnaryOperator oper, Value &rightValue);
  Value evalInfixExpression(BinaryOperator oper, const Value &leftValue,
                            const Value &rightValue);
  Value evalIfStatement(int index);
  Value evalBlockStatement(int index);
  Value evalVarStatement(int index);
};
