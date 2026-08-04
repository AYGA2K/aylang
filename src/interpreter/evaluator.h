#pragma once
#include "parser/expression.h"
#include "parser/parser.h"
#include "value.h"

struct Evaluator {
  ParserResult parserResult;
  // Returns the value of the last statement.
  Value evalStatements();
  Value evalStatement(int index);
  Value evalExpression(int index);
  Value evalPrefixExpression(UnaryOperator oper, Value rightValue);
  Value evalInfixExpression(BinaryOperator oper, Value leftValue,
                            Value rightValue);
  Value evalIfStatement(int index);
  Value evalBlockStatement(int index);
};
